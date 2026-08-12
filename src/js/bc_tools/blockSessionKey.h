#pragma once
// Класс — только данные и сравнение
class BlockSessionKey
{
public:
    HashType    prevRootHash_;
    NODE_id     nodeName_;
    uint64_t    timestampUs_;

    bool operator<(const BlockSessionKey& o) const
    {
        if (prevRootHash_ != o.prevRootHash_) return prevRootHash_ < o.prevRootHash_;
        if (nodeName_ != o.nodeName_)         return nodeName_ < o.nodeName_;
        return timestampUs_ < o.timestampUs_;
    }

    bool operator==(const BlockSessionKey& o) const
    {
        return prevRootHash_ == o.prevRootHash_
            && nodeName_ == o.nodeName_
            && timestampUs_ == o.timestampUs_;
    }

    std::string toString() const
    {
        return prevRootHash_.toHex() + ":" + nodeName_ + ":" + std::to_string(timestampUs_);
    }
};

// Сериализация — свободные функции, без friend
inline outBuffer& operator<<(outBuffer& buf, const BlockSessionKey& key)
{
    buf << key.prevRootHash_;
    buf << key.nodeName_;
    buf << key.timestampUs_;
    return buf;
}

inline inBuffer& operator>>(inBuffer& buf, BlockSessionKey& key)
{
    buf >> key.prevRootHash_;
    buf >> key.nodeName_;
    buf >> key.timestampUs_;
    return buf;
}