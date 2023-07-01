#include <h5DataLoader.h>
#include <hdf5/hdf5.h>
#include <algorithm>
#include <iostream>
#include <limits>
#include <type_traits>
#include <vector>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

enum : int32_t { INVALID_VALUE = -std::numeric_limits<int32_t>::max() };

template <typename Derived>
class H5Object
{
public:
    H5Object() { mId = -1; }
    ~H5Object() { Close(); }

    H5Object(H5Object &&o)
    :   mId(o.mId)
    {
        o.mId = -1;
    }

    H5Object & operator =(H5Object &&o)
    {
        if (&o != this)
        {
            mId = o.mId;
            o.mId = -1;
        }
        return *this;
    }

    H5Object(H5Object const &) = delete;
    H5Object & operator =(H5Object const &) = delete;

    hid_t GetId() const { return mId; }

    operator bool() const { return 0 <= mId; }

    void Close()
    {
        if (*this)
            Derived::Destroy(mId);
    }

    int32_t GetAttribute(char const *name)
    {
        int32_t rtn = INVALID_VALUE;
        if (bool(*this) and name != nullptr)
        {
            hid_t id = H5Aopen(mId, name, H5P_DEFAULT);
            if (0 <= id)
            {
                int32_t buff = 0;
                if (0 <= H5Aread(id, H5T_STD_I32LE, &buff))
                    rtn = buff;
                H5Aclose(id);
            }
        }
        return rtn;
    }

protected:
    hid_t mId;
};

class H5File : public H5Object<H5File>
{
    friend H5Object<H5File>;

    static void Destroy(hid_t id) { H5Fclose(id); }

public:
    bool Open(char const *name)
    {
        Close();
        if (name != nullptr)
            mId = H5Fopen(name, H5F_ACC_RDONLY, H5P_DEFAULT);
        return *this;
    }
};

class H5Group : public H5Object<H5Group>
{
    friend H5Object<H5Group>;

    static void Destroy(hid_t id) { H5Gclose(id); }

public:
    bool Open(hid_t loc, char const *name)
    {
        Close();
        if (0 <= loc and name != nullptr)
            mId = H5Gopen(loc, name, H5P_DEFAULT);
        return *this;
    }

    template <typename Visitor>
    void Iterate(Visitor const &visitor) const
    {
        if (*this)
        {
            auto callback = [](hid_t id, char const *name, H5L_info_t const *, void *data) -> herr_t
            {
                (*static_cast<Visitor *>(data))(id, name);
                return herr_t();
            };

            H5Literate(mId, H5_INDEX_NAME, H5_ITER_INC, nullptr, callback, const_cast<Visitor *>(&visitor));
        }
    }
};

class H5DataType : public H5Object<H5DataType>
{
    friend H5Object<H5DataType>;

    static void Destroy(hid_t id) { H5Tclose(id); }

public:
    bool Create(size_t size)
    {
        Close();
        mId = H5Tcreate(H5T_COMPOUND, size);
        return *this;
    }

    bool Insert(char const *name, size_t offset, hid_t type)
    {
        return name != nullptr and (*this) and 0 <= H5Tinsert(mId, name, offset, type);
    }
};

class H5Dataset : public H5Object<H5Dataset>
{
    friend H5Object<H5Dataset>;

    static void Destroy(hid_t id) { H5Dclose(id); }

public:
    bool Open(hid_t loc, char const *name)
    {
        Close();
        if (0 <= loc and name != nullptr)
            mId = H5Dopen(loc, name, H5P_DEFAULT);
        return *this;
    }

    size_t length() const
    {
        if (*this)
        {
            hid_t space = H5Dget_space(mId);
            hsize_t len = 0;
            H5Sget_simple_extent_dims(space, &len, nullptr);
            H5Sclose(space);
            return len;
        }
        return 0u;
    }

    std::vector<int32_t> Read() const
    {
        std::vector<int32_t> data;
        if (*this)
        {
            size_t len = length();
            if (0 < len)
            {
                data.resize(len);
                if (H5Dread(mId, H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data.data()) < 0)
                    data.clear();
            }
        }
        return std::move(data);
    }
};

static Time GetDateTime(int32_t date)
{
    return Time(date / 10000, date / 100 % 100, date % 100);
}

static std::vector<Segment> ReadSegments(H5Group &group)
{
    H5Dataset segmentsDataset;
    if (not segmentsDataset.Open(group.GetId(), "TradingSegments"))
        return std::vector<Segment>();

    std::vector<int32_t> segments = segmentsDataset.Read();
    if (segments.size() % 3 != 0)
    {
        std::cout << "Invalid input data, unexpected number of trading ranges.\n";
        return std::vector<Segment>();
    }

    std::vector<Segment> rtn;
    rtn.reserve(segments.size());
    for (size_t i = 0u; i < segments.size(); i += 3u)
    {
        Time date = GetDateTime(segments[i] / 10);
        if (date == DistantPast())
        {
            std::cout << "Invalid input data, unexpected trading day " << segments[i] << "\n";
            return std::vector<Segment>();
        }
        rtn.push_back(Segment{ date + Milliseconds(segments[i + 1u]), date + Milliseconds(segments[i + 2u]) });
    }
    return std::move(rtn);
}

static std::unique_ptr<char []> CopyCString(char const *cstr)
{
    size_t len = strlen(cstr) + 1;
    std::unique_ptr<char []> str(new char[len]);
    memcpy(str.get(), cstr, len);
    return std::move(str);
}

static std::vector<std::unique_ptr<char[]>> GetSubgroupNames(H5Group &group)
{
    std::vector<std::unique_ptr<char []>> names;
    group.Iterate([&names](hid_t, char const *name) { names.push_back(CopyCString(name)); });
    return std::move(names);
}

static int32_t CalcDate(char const *tradingDay)
{
    if (tradingDay == nullptr)
        return 0;

    int32_t date = 0;
    while (*tradingDay != 0)
    {
        if (*tradingDay < '0' or '9' < *tradingDay)
            return 0;

        date = date * 10 + (*tradingDay++ - int32_t('0'));
    }
    return date;
}

static std::vector<PointData> GetPointData(char const *sname, int32_t date, std::vector<int32_t> data)
{
    Time time = GetDateTime(date / 10);
    if (time == DistantPast())
    {
        std::cout << "Invalid input data, unexpected trading day. Series=" << sname
            << " Date=" << date << "\n";
        return std::vector<PointData>();
    }

    if (data.size() % 2 != 0)
    {
        std::cout << "Invalid input data, unexpected number of integer values. Series=" << sname
            << " Date=" << date << "\n";
        return std::vector<PointData>();
    }

    std::vector<PointData> series;
    series.reserve(data.size());
    for (size_t i = 0; i < data.size(); i += 2u)
        series.push_back(PointData{ time + Milliseconds(data[i]), Fixpoint(data[i + 1], Fixpoint::REPRESENTATION) });
    return std::move(series);
}

H5DataLoader::H5DataLoader(char const *fname)
{
    H5File file;
    if (not file.Open(fname))
    {
        std::cout << "Invalid input data, failed to open HDF5 file. FileName=" << fname << "\n";
        return;
    }

    H5Group plot;
    if (not plot.Open(file.GetId(), "Plot"))
    {
        std::cout << "Invalid input data, failed to open base group 'Plot'.\n";
        return;
    }

    if (not InsertSegments(Milliseconds(1), ReadSegments(plot)))
    {
        std::cout << "Invalid input data, failed to load trading segments.\n";
        return;
    }

    std::vector<std::unique_ptr<char []>> groupNames = GetSubgroupNames(plot);
    if (groupNames.empty())
    {
        std::cout << "Invalid input data, failed to find series groups.\n";
        return;
    }

    for (std::unique_ptr<char []> &groupName : groupNames)
    {
        if (strcmp(groupName.get(), "TradingSegments") == 0)
            continue;

        char const *gname = groupName.get();
        H5Group group;
        if (not group.Open(plot.GetId(), gname))
        {
            std::cout << "Invalid input data, failed to open series group. Group=" << gname << "\n";
            return;
        }

        std::vector<std::unique_ptr<char []>> seriesNames = GetSubgroupNames(group);
        for (std::unique_ptr<char []> &seriesName : seriesNames)
        {
            char const *sname = seriesName.get();
            H5Group series;
            if (not series.Open(group.GetId(), sname))
            {
                std::cout << "Invalid input data, failed to open series. Group=" << gname << " Series=" << sname << "\n";
                return;
            }

            int32_t centre = series.GetAttribute("Centre");
            if (centre == INVALID_VALUE)
            {
                std::cout << "Invalid input data, failed to load series centre. Series=" << sname << "\n";
                return;
            }

            int32_t shape = series.GetAttribute("Shape");
            if (shape == INVALID_VALUE)
            {
                std::cout << "Invalid input data, failed to load series shape. Series=" << sname << "\n";
                return;
            }

            if (shape != Shape::Curve and shape != Shape::Spike and shape != Shape::Step)
            {
                std::cout << "Invalid input data, unexpected series shape. Series=" << sname
                    << " Shape=" << shape << "\n";
                return;
            }

            Series s;
            if (strcmp(sname, gname) == 0)
            {
                s.mName = std::move(groupName);
                s.mGroup = "";
            }
            else
            {
                s.mName = std::move(seriesName);
                s.mGroup = gname;
            }

            int32_t prevDate = -1;
            std::vector<PointData> data;
            for (std::unique_ptr<char []> const &tradingDay : GetSubgroupNames(series))
            {
                int32_t date = CalcDate(tradingDay.get());
                if (date < 197001010 or 220001010 < date)
                {
                    std::cout << "Invalid input data, unexpected trading day. Series=" << sname
                        << " TradingDay=" << tradingDay.get() << "\n";
                    return;
                }

                if (date <= prevDate)
                {
                    std::cout << "Invalid input data, trading day is not greater than previous. Series=" << sname
                        << " TradingDay=" << tradingDay.get() << " PrevTradingDay=" << prevDate << "\n";
                    return;
                }
                prevDate = date;

                H5Dataset dataset;
                if (not dataset.Open(series.GetId(), tradingDay.get()))
                {
                    std::cout << "Invalid input data, failed to open dataset. Series=" << sname
                        << " TradingDay=" << tradingDay.get() << "\n";
                    return;
                }

                std::vector<PointData> seriesData = GetPointData(sname, date, dataset.Read());
                if (seriesData.empty())
                    return;
                data.insert(data.end(), seriesData.data(), seriesData.data() + seriesData.size());
            }

            if (centre == std::numeric_limits<int32_t>::max())
            {
                s.mAxisCentre = Fixpoint::Min();
            }
            else if (centre + 1 == std::numeric_limits<int32_t>::max())
            {
                auto Comp = [](PointData const &o1, PointData const &o2) { return o1.mValue < o2.mValue; };
                auto pair = std::minmax_element(data.begin(), data.end(), Comp);
                s.mAxisCentre = (pair.first->mValue + pair.second->mValue) / 2;
            }
            else
            {
                s.mAxisCentre = Fixpoint(centre, Fixpoint::REPRESENTATION);
            }

            s.mData = LoadPoints(sname, shape, s.mAxisCentre, data);
            if (s.mData.empty())
                return;
            mSeries.push_back(std::move(s));
        }
    }

    mDisplayRange = plot.GetAttribute("DisplayRange");
    if (mDisplayRange == INVALID_VALUE)
    {
        std::cout << "Invalid input data, failed to load display range.\n";
        mDisplayRange = 0;
    }
}

H5DataLoader::~H5DataLoader()
{
}

void H5DataLoader::RetrieveSeries(ISeriesRetriever &retriever) const
{
    for (Series const &series : mSeries)
        retriever.OnSeries(series.mName.get(), series.mGroup, series.mAxisCentre, &series.mData[0], &series.mData[series.mData.size()]);
}

