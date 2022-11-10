#ifndef MATRIX_H
#define MATRIX_H
#include <functional>

template <class T, int rows, int cols>
class matrix
{

private:
    std::array<T, unsigned(rows*cols)> data;

    size_t index(const int row, const int col) const
    {
        assert(row >= 0 && col >= 0 && row < rows && col < cols);
        return size_t(row*cols + col);
    }

public:

    matrix() : data({{0}})
    {}

    matrix(const std::array<T, unsigned(rows*cols)> &m) : data({{0}})
    {
        data = m;
    }

    int get_rows() const
    {
        return rows;
    }

    int get_cols() const
    {
        return cols;
    }

    T & at(const int row, const int col)
    {
        return data[index(row, col)];
    }

    T at(const int row, const int col) const
    {
        return data[index(row, col)];
    }

    bool operator ==(const matrix<T, rows, cols> &m) const
    {
        for(int row = 0; row < rows; ++row)
            for(int col = 0; col < cols; ++col)
                if(data[index(row, col)] != m.at(row, col))
                    return false;
        return true;
    }

    bool roughly_equal(const matrix<T, rows, cols> &m, T tolerance) const
    {
        for(int row = 0; row < rows; ++row)
            for(int col = 0; col < cols; ++col)
                if(std::abs(data[index(row, col)] -
                            m.at(row, col)) > tolerance)
                    return false;
        return true;
    }

    bool operator !=(const matrix<T, rows, cols> &m) const
    {
        return !(*this == m);
    }

    matrix operator +(const matrix<T, rows, cols> &m) const
    {
        matrix<T, rows, cols> ans;
        for(int row = 0; row < rows; ++row)
            for(int col = 0; col < cols; ++col)
                ans.at(row, col) = data[index(row, col)] + m.at(row, col);
        return ans;
    }

    matrix operator -(const matrix<T, rows, cols> &m) const
    {
        matrix<T, rows, cols> ans;
        for(int row = 0; row < rows; ++row)
            for(int col = 0; col < cols; ++col)
                ans.at(row, col) = data[index(row, col)] - m.at(row, col);
        return ans;
    }

    template<int N>
    matrix<T, rows, N> operator *(const matrix<T, cols, N> &m) const
    {
        matrix<T, rows, N> ans;
        for(int col = 0; col < N; ++col)
            for(int row = 0; row < rows; ++row)
            {
                T sum = 0;
                for(int i = 0; i < cols; ++i)
                    sum += data[index(row, i)]*m.at(i, col);
                ans.at(row, col) = sum;
            }
        return ans;
    }

    void transform_each(const std::function<T(T)> &foo)
    {
        for(int row = 0; row < rows; ++row)
            for(int col = 0; col < cols; ++col)
                data[index(row, col)] = foo(data[index(row, col)]);
    }
};
#endif
