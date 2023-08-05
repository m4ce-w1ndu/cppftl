#ifndef FDT_RANDOM_MATRIX_H
#define FDT_RANDOM_MATRIX_H

#include <cmath>
#include <utility>
#include <type_traits>
#include <initializer_list>
#include <algorithm>
#include <fdt/exception.h>
#include <fdt/iterator.h>

namespace fdt {
    /**
     * @brief Implementation of a template matrix class.
     * An allocator could be provided as defaulted template
     * parameter.
     * @tparam T type of the container.
     * @tparam Rows Number of rows.
     * @tparam Cols Number of columns.
     * @tparam Allocator allocator.
     */
    template <typename T,
            std::size_t Rows, std::size_t Cols,
            class Allocator = std::allocator<T>>
    class matrix {
    public:
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;
        using iterator = random_access_iterator<T>;
        using const_iterator = const random_access_iterator<T>;
        using reverse_iterator = reverse_iterator<random_access_iterator<T>>;
        using const_reverse_iterator =
                const reverse_iterator;
        using allocator_type = Allocator;

        /**
         * Default constructor. Constructs an empty matrix object.
         */
        constexpr matrix()
    		: _data(allocator_traits::allocate(_alloc, Rows * Cols)) {}

        /**
         * Initializing constructor. Constructs a new matrix object
         * using the initializer list to initialize its values.
         * @param init std::initializer_list<Ty> containing matrix
         * values.
         */
        constexpr matrix(std::initializer_list<T> init)
        {
            _data = allocator_traits::allocate(_alloc, Rows * Cols);
            if (init.size() == Rows * Cols) {
                size_t i = 0;
                for (auto& x : init) {
                    _data[i] = x;
                    ++i;
                }
            }
            else throw matrix_out_of_range();
        }

        /**
         * Copy constructor. Copies another matrix object of the same
         * type into the current instance, preserving the content.
         * @param other matrix object with same type and same
         * size.
         */
        constexpr matrix(const matrix& other)
        {
            _data = allocator_traits::allocate(_alloc, Rows * Cols);
            std::copy(other._data, other._data + (Rows * Cols - 1), _data);
        }

        /**
         * Move constructor. Moves data from another matrix object,
         * provided that references to the other object are expired.
         * @param other matrix object with same type and same
         * size.
         */
        constexpr matrix(matrix&& other) noexcept
            : _data(std::move(other._data)) { other._data = nullptr; }

        ~matrix()
        {
            allocator_traits::deallocate(_alloc, _data, Rows * Cols);
        }

        constexpr allocator_type get_allocator() const noexcept
        {
            return _alloc;
        }

        /**
         * Fills the matrix with a given value.
         * @param val a value of type matrix::value_type.
         */
        constexpr void fill(const T& val)
        {
            std::fill_n(_data, Rows * Cols, val);
        }

        /**
         * Returns the size of the matrix.
         * @return pair containing the size of the matrix.
         */
        constexpr auto size() const noexcept
        {
            return std::make_pair<size_t, size_t>(Rows, Cols);
        }

        /**
         * Returns the number of rows.
         * @return size_t containing the number of rows.
         */
        constexpr auto rows() const { return Rows; }

        /**
         * Returns the number of columns.
         * @return size_t containing the number of columns.
         */
        constexpr auto cols() const { return Cols; }

        /**
         * Returns a boolean indicating the current state of the matrix.
         * @return true if the size of the matrix computes to 0, false
         * otherwise.
         */
        [[nodiscard]]
        constexpr bool empty() const noexcept { return (Rows * Cols) == 0; }

        /**
         * Returns an iterator to the beginning of the matrix.
         * @return fdt::rand_iterator to the beginning of the matrix.
         */
        constexpr iterator begin() { return iterator(_data); }

        constexpr const_iterator begin() const { return iterator(_data); }

        /**
         * Returns an iterator to the end of the matrix.
         * @return fdt::rand_iterator to the end of the matrix.
         */
        constexpr iterator end() { return iterator(_data + Rows * Cols); }

        constexpr const_iterator end() const { return iterator(_data + Rows * Cols); }

        /**
         * Returns the underlying data structure containing matrix
         * data.
         * @return pointer to the underlying data structure. Data is
         * linearized for easier memory access, which needs notational
         * conversion of indices if access through the underlying data
         * structure is needed.
         */
        constexpr auto data() { return _data; }

        constexpr auto data() const { return _data; }

        /**
         * Returns a reference to the element of the matrix indexed
         * by row and column.
         * @param row current row.
         * @param col current column.
         * @return value_type reference to the indexed object.
         */
        constexpr reference
        operator()(size_t row, size_t col) noexcept
        {
            return _data[Cols * row + col];
        }

        constexpr const_reference
        operator()(size_t row, size_t col) const noexcept
        {
            return _data[Cols * row + col];
        }

        /**
         * Returns a reference to the element of the matrix indexed
         * by row and col. Every access is bounds-checked and may throw
         * matrix_out_of_range if any of the immediate values pertaining
         * to row or col is out of bounds.
         * @param row current row.
         * @param col current column.
         * @return value_type reference to the indexed element.
         */
        constexpr reference
    	at(size_t row, size_t col)
        {
            if (row >= Rows || col >= Cols)
                throw matrix_out_of_range("matrix indices out of range");
            return _data[Cols * row + col];
        }

    	constexpr const_reference
    	at(size_t row, size_t col) const
        {
            if (row >= Rows || col >= Cols)
                throw matrix_out_of_range("matrix indices out of range");
            return _data[Cols * row + col];
        }

        constexpr void swap(matrix& other) noexcept
        {
            std::swap(other._data, _data);
            std::swap(other._alloc, _alloc);
        }

        /**
         * Calculates the determinant of an arithmetic compliant matrix
         * by using the iterative method.
         * @return double determinant value.
         */
        double determinant() const
        {
            if (Rows != Cols)
                throw matrix_non_square("determinant is not defined"
                                        "for non square matrices.");

            // new allocator
            using local_alloc = std::allocator<double>;
            local_alloc alloc{};
            using alloc_traits = std::allocator_traits<local_alloc>;
            const size_t N = Rows;

            const double SMALL = 1.0E-30;
            double* mat = alloc_traits::allocate(alloc, N * N);

            // copying matrix with cast
            std::transform(_data, _data + Rows * Cols, mat,
                           [](T x) { return static_cast<double>(x); });
            auto det = 1.0;

            // Row operations for i = 0, ..., n - 2
            for (size_t i = 0; i < N - 1; ++i) {
                // Partial pivot: find row r below with largest element in column i
                size_t r = i;
                double max = std::abs(mat[N * i + i]);
                for (size_t k = i + 1; k < N; ++k) {
                    double val = std::abs(mat[N * k + i]);
                    if (val > max) {
                        max = val;
                        r = k;
                    }
                }
                if (r != i) {
                    for (size_t j = i; j < N; ++j)
                        std::swap(mat[N * i + j], mat[N * r + j]);
                    det = -det;
                }

                // Row operations to calculate upper-triangular
                const double pivot = mat[N * i + i];
                // Singular matrix
                if (std::abs(pivot) < SMALL) {
                    alloc_traits::deallocate(alloc, mat, N * N);
                    return 0.0;
                }

                // On lower rows
                for (size_t s = i + 1; s < N; ++s) {
                    // Multiple of row i to clear element in i-th column
                    const double multiple = mat[N * s + i] / pivot;
                    for (size_t j = i; j < N; ++j)
                        mat[N * s + j] -= multiple * mat[N * i + j];
                }
                det *= pivot;
            }
            det *= mat[N * (N - 1) + (N - 1)];

            alloc_traits::deallocate(alloc, mat, N * N);
            return det;
        }

        /**
         * Generates the transpose of the current matrix object.
         * @return new matrix object containing the transpose of the
         * original matrix.
         */
        constexpr auto transpose() const
        {
            matrix<T, Cols, Rows> mat;
            for (size_t i = 0; i < Rows; ++i)
                for (size_t j = 0; j < Cols; ++j)
                    mat(j, i) = this->operator()(i, j);
            return mat;
        }

        constexpr matrix& operator=(const matrix& other)
        {
            if (&other == this) return *this;
        	
            matrix copy(other);
            copy.swap(*this);
            return *this;
        }

        constexpr matrix& operator=(matrix&& other) noexcept
        {
            if (&other == this) return *this;
        	
            matrix copy(std::move(other));
            copy.swap(*this);
            return *this;
        }

		constexpr matrix& operator=(std::initializer_list<T> init)
        {
            matrix copy(init);
            copy.swap(*this);
            return *this;
        }
    private:
        allocator_type _alloc;
        T* _data;
        using allocator_traits = std::allocator_traits<Allocator>;
    };

	template<typename Ty, std::size_t Rows, std::size_t Cols>
	constexpr bool operator==(
        const matrix<Ty, Rows, Cols>& l,
		const matrix<Ty, Rows, Cols>& r)
	{
        for (size_t i = 0; i < Rows * Cols; ++i)
            if (*(l.data() + i) != *(r.data() + i)) return false;
        return true;
	}

    template<typename Ty, std::size_t Rows, std::size_t Cols>
    constexpr bool operator!=(
        const matrix<Ty, Rows, Cols>& l,
        const matrix<Ty, Rows, Cols>& r)
    {
        return !(l == r);
    }

	template<typename Ty, std::size_t Rows, std::size_t Cols>
    constexpr auto operator+(
        const matrix<Ty, Rows, Cols>& l, 
        const matrix<Ty, Rows, Cols>& r)
	{
        matrix<Ty, Rows, Cols> sum;
        for (size_t i = 0; i < Rows * Cols; ++i)
            *(sum.data() + i) = *(l.data() + i) + *(r.data() + i);
        return sum;
	}

	template<typename Ty, std::size_t Rows, std::size_t Cols>
    constexpr auto operator-(
        const matrix<Ty, Rows, Cols>& l,
        const matrix<Ty, Rows, Cols>& r)
	{
        matrix<Ty, Rows, Cols> sub;
        for (size_t i = 0; i < Rows * Cols; ++i)
            *(sub.data() + i) = *(l.data() + i) - *(r.data() + i);
        return sub;
	}

	template<typename Ty, std::size_t Rows, std::size_t Cols>
    constexpr matrix<Ty, Rows, Cols>& operator+=(
        matrix<Ty, Rows, Cols>& l, const matrix<Ty, Rows, Cols>& r)
	{
        for (size_t i = 0; i < Rows * Cols; ++i)
            *(l.data() + i) += *(r.data() + i);
        return l;
	}

    template<typename Ty, std::size_t Rows, std::size_t Cols>
    constexpr matrix<Ty, Rows, Cols>& operator-=(
        matrix<Ty, Rows, Cols>& l, const matrix<Ty, Rows, Cols>& r)
    {
        for (size_t i = 0; i < Rows * Cols; ++i)
            *(l.data() + i) -= *(r.data() + i);
        return l;
    }
}

#endif
