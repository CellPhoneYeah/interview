class EllConnBase
{
public:
    virtual int getSock() = 0;
    virtual void bindEVQ(int evq) = 0;
    virtual void close() = 0;
};