/**
* An implementation of std::vector (without costum allocator) such that all the avaialbe operators are lazy evaluated (via expression template proxies).
* The only requirement from the user, is that the object held by the vector will have the required operators defined,
* and that numerical/bit operators shall have both the regular (+,/,-,...) and the assignable (+=, /=, -=,....) form defined.
*
* Dan Israel Malta
**/
#pragma once

#include <vector>
#include <assert.h>
#include <type_traits>
#include <utility>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <stdexcept>

/**
* Lazy Vector exteds std::vector with lazy evaluated operators.
**/
namespace Lazy {

    /**
    * \brief a binary expression
    *
    * @param {LeftExpr,  in} left side of expression
    * @param {BinaryOp,  in} binary operation
    * @param {RightExpr, in} right side of expression
    **/
    template<typename LeftExpr, typename BinaryOp, typename RightExpr> class BinaryExpression {
        // properties
        private:
            const LeftExpr m_left;
            const RightExpr m_right;

        // constructors
        public:
            // prohibit empty constructor
            BinaryExpression() = delete;

            // element wise constructor
            BinaryExpression(LeftExpr l, RightExpr r) : m_left(std::forward<LeftExpr>(l)), m_right(std::forward<RightExpr>(r)) {}

            // expression can not be copied...
            BinaryExpression(BinaryExpression const&) = delete;
            BinaryExpression& operator =(BinaryExpression const&) = delete;

            // ...only moved
            BinaryExpression(BinaryExpression&&) = default;
            BinaryExpression& operator =(BinaryExpression&&) = default;

        // binary operators overload
        public:

#define CREATE_BINARY_EXPRESSION_OPERATOR(xi_operator)                                                                                                                                                      \
        template<typename RE> auto operator xi_operator(RE&& re) const -> BinaryExpression<const BinaryExpression<LeftExpr, BinaryOp, RightExpr>&, BinaryOp, decltype(std::forward<RE>(re))> {  \
            return BinaryExpression<const BinaryExpression<LeftExpr, BinaryOp, RightExpr>&, BinaryOp, decltype(std::forward<RE>(re))>(*this, std::forward<RE>(re));                             \
        }

        CREATE_BINARY_EXPRESSION_OPERATOR(+=);
        CREATE_BINARY_EXPRESSION_OPERATOR(-=);
        CREATE_BINARY_EXPRESSION_OPERATOR(*=);
        CREATE_BINARY_EXPRESSION_OPERATOR(/=);
        CREATE_BINARY_EXPRESSION_OPERATOR(|=);
        CREATE_BINARY_EXPRESSION_OPERATOR(&=);
        CREATE_BINARY_EXPRESSION_OPERATOR(^=);
        CREATE_BINARY_EXPRESSION_OPERATOR(<<=);
        CREATE_BINARY_EXPRESSION_OPERATOR(>>=);
        CREATE_BINARY_EXPRESSION_OPERATOR(+);
        CREATE_BINARY_EXPRESSION_OPERATOR(-);
        CREATE_BINARY_EXPRESSION_OPERATOR(*);
        CREATE_BINARY_EXPRESSION_OPERATOR(/);
        CREATE_BINARY_EXPRESSION_OPERATOR(|);
        CREATE_BINARY_EXPRESSION_OPERATOR(&);
        CREATE_BINARY_EXPRESSION_OPERATOR(^);
        CREATE_BINARY_EXPRESSION_OPERATOR(<<);
        CREATE_BINARY_EXPRESSION_OPERATOR(>>);
        CREATE_BINARY_EXPRESSION_OPERATOR(&&);
        CREATE_BINARY_EXPRESSION_OPERATOR(||);
        CREATE_BINARY_EXPRESSION_OPERATOR(==);
        CREATE_BINARY_EXPRESSION_OPERATOR(!=);
        CREATE_BINARY_EXPRESSION_OPERATOR(<);
        CREATE_BINARY_EXPRESSION_OPERATOR(<=);
        CREATE_BINARY_EXPRESSION_OPERATOR(>);
        CREATE_BINARY_EXPRESSION_OPERATOR(>=);
#undef CREATE_BINARY_EXPRESSION_OPERATOR

        // getters
        public:

            // expression left hand side
            auto le()       -> typename std::add_lvalue_reference<                        LeftExpr>       ::type { return m_left; }
            auto le() const -> typename std::add_lvalue_reference<typename std::add_const<LeftExpr>::type>::type { return m_left; }

            // expression right hand side
            auto re()       -> typename std::add_lvalue_reference<                        RightExpr>       ::type { return m_right; }
            auto re() const -> typename std::add_lvalue_reference<typename std::add_const<RightExpr>::type>::type { return m_right; }

            // [] overload to get expression at a specific index
            auto operator [](std::size_t index) const -> decltype(BinaryOp::apply(this->le()[index], this->re()[index])) {
                return BinaryOp::apply(le()[index], re()[index]);
            }
    };

    /**
    * binary operations
    **/
    namespace BinaryOperations {

        // numerical/bit operator overloading
#define CREATE_BINARY_OPERATION(xi_name, xi_operator, xi_assign_operator)                               \
    template<typename T> struct xi_name {                                                               \
        static T apply(const T& a, const T& b) { return a xi_operator b; }                              \
        static T apply(T&& a,      const T& b) { a xi_assign_operator b; return std::move(a); }         \
        static T apply(const T& a, T&& b)      { b xi_assign_operator a; return std::move(b); }         \
        static T apply(T&& a,      T&& b)      { a xi_assign_operator b; return std::move(a); }         \
    }

        CREATE_BINARY_OPERATION(ADD, +, +=);
        CREATE_BINARY_OPERATION(SUB, -, -=);
        CREATE_BINARY_OPERATION(MUL, *, *=);
        CREATE_BINARY_OPERATION(DIV, / , /=);
        CREATE_BINARY_OPERATION(LOR, | , |=);
        CREATE_BINARY_OPERATION(LAND, &, &=);
        CREATE_BINARY_OPERATION(LXOR, ^, ^=);
        CREATE_BINARY_OPERATION(SHL, << , <<=);
        CREATE_BINARY_OPERATION(SHR, >> , >>=);
#undef CREATE_BINARY_OPERATION

        // relation/logical operator overloading
#define CREATE_BINARY_OPERATION(xi_name, xi_operator)                               \
    template<typename T> struct xi_name {                                           \
        static T apply(const T& a, const T& b) { return a xi_operator b; }          \
        static T apply(T&& a,      const T& b) { return a xi_operator b; }          \
        static T apply(const T& a, T&& b)      { return a xi_operator b; }          \
        static T apply(T&& a,      T&& b)      { return a xi_operator b; }          \
    }

        CREATE_BINARY_OPERATION(AND, &&);
        CREATE_BINARY_OPERATION(OR, ||);
        CREATE_BINARY_OPERATION(EQ, ==);
        CREATE_BINARY_OPERATION(NEQ, !=);
        CREATE_BINARY_OPERATION(LT, <);
        CREATE_BINARY_OPERATION(LE, <=);
        CREATE_BINARY_OPERATION(GT, >);
        CREATE_BINARY_OPERATION(GE, >=);
#undef CREATE_BINARY_OPERATION

    };

    
    /**
    * \brief lazy element-wise evaluated vector
    * 
    * @param {T, in} vector underlying type
    **/
    template<typename T> class Vector {
        // vector maximal possible size (seems like a reasonable number...)
        const std::size_t MaximumPossibleSize{ 1'000'000'000 };

        // properties
        private:
            std::size_t m_reservedSize{ 4 };    // vector reserved size
            std::size_t m_size{ 0 };            // vector size
            T *m_data;                          // data holder

        // internal methods
        private:

            // reallocate vector (used when increasing vector size beyond its current size)
            inline void reallocate() {
                T *temp = new T[m_reservedSize];
                memcpy(temp, m_data, m_size * sizeof(T));
                delete[] m_data;
                m_data = temp;
            }

        // types
        public:
            using iterator = T*;
            using const_iterator = const T*;
            using reverse_iterator = std::reverse_iterator<iterator>;
            using const_reverse_iterator = std::reverse_iterator<const_iterator>;
            using difference_type = ptrdiff_t;

        // constructors
        public:

            // empty constructor
            Vector() noexcept {
                m_data = new T[m_reservedSize];
            }

            // construct a vector by its size
            explicit constexpr Vector(const std::size_t xi_size) {
                // new size's
                m_reservedSize = 2 * xi_size;
                m_size = xi_size;

                // allocate and fill data container
                m_data = new T[m_reservedSize];
                for (std::size_t i{}; i < xi_size; ++i) {
                    m_data[i] = T{};
                }
            }

            // construct a vector by its size and initial value
            explicit constexpr Vector(const std::size_t xi_size, const T& xi_value) {
                // new size's
                m_reservedSize = 2 * xi_size;
                m_size = xi_size;

                // allocate and fill data container
                m_data = new T[m_reservedSize];
                for (std::size_t i{}; i < xi_size; ++i) {
                    m_data[i] = xi_value;
                }
            }

            // construct a vector by iterators
            explicit constexpr Vector(T* xi_first, T* xi_last) {
                const std::size_t len{ xi_last - xi_first };

                // new size's
                m_reservedSize = 2 * len;
                m_size = len;

                // allocate and fill data container
                m_data = new T[m_reservedSize];
                for (std::size_t i{}; i < len; ++i, ++xi_first) {
                    m_data[i] = *xi_first;
                }
            }

            // construct a vector from initializer list
            explicit constexpr Vector(std::initializer_list<T> xi_list) {
                // new size's
                m_reservedSize = 2 * xi_list.size();
                m_size = xi_list.size();

                // allocate and fill data container
                m_data = new T[m_reservedSize];
                for (auto &item : xi_list) {
                    m_data[m_size++] = item;
                }
            }

            // copy constructor
            Vector(const Vector& xi_other) {
                // new size's
                m_reservedSize = xi_other.m_reservedSize;
                m_size = xi_other.m_size;

                // allocate and fill data container
                m_data = new T[m_reservedSize];
                for (std::size_t i{}; i < xi_other.m_size; ++i) {
                    m_data[i] = xi_other.m_data[i];
                }
            }

            // move constructor
            Vector(Vector&& xi_other) noexcept {
                // new size's
                m_reservedSize = xi_other.m_reservedSize;
                m_size = xi_other.m_size;

                // allocate and fill data container
                m_data = new T[m_reservedSize];
                for (std::size_t i{}; i < xi_other.m_size; ++i) {
                    m_data[i] = std::move(xi_other.m_data[i]);
                }

                xi_other.m_data = nullptr;
                xi_other.m_size = 0;
            }

            // destructor
            ~Vector() {
                delete[] m_data;
            }

            // copy assignment
            Vector& operator = (const Vector& xi_other) {
                // allocate
                m_size = xi_other.m_size;
                if (m_reservedSize < xi_other.m_size) {
                    m_reservedSize = 2 * xi_other.m_size;
                    reallocate();
                }

                // fill data container
                for (std::size_t i{}; i < xi_other.m_size; ++i) {
                    m_data[i] = xi_other.m_data[i];
                }
            }

            // move assignment
            Vector& operator = (Vector&& xi_other) {
                // allocate
                m_size = xi_other.m_size;
                if (m_reservedSize < xi_other.m_size) {
                    m_reservedSize = 2 * xi_other.m_size;
                    reallocate();
                }

                // fill data container
                for (std::size_t i{}; i < xi_other.m_size; ++i) {
                    m_data[i] = std::move(xi_other.m_data[i]);
                }

                xi_other.m_data = nullptr;
                xi_other.m_size = 0;
            }

            // construct from a binary expression
            template<typename LE, typename Op, typename RE> Vector(BinaryExpression<LE, Op, RE>&& xi_expression) {
                for (std::size_t i{}; i < m_size; ++i) {
                    this->operator[](i) = xi_expression[i];
                }
            }

            // assign from a (right) expression
            template<typename RightExpr> Vector& operator =(RightExpr&& xi_expression) {
                for (std::size_t i{}; i < m_size; ++i) {
                    this->operator[](i) = xi_expression[i];
                }
                return *this;
            }

            // assign from initializer list
            constexpr Vector& operator = (std::initializer_list<T> xi_list) {
                // allocation               
                if (m_reservedSize < xi_list.size()) {
                    m_reservedSize = 2 * xi_list.size();
                    reallocate();
                }

                // fill data container
                m_size = 0;
                for (auto& item : xi_list) {
                    m_data[m_size++] = item;
                }
            }
            // assigns new contents to the vector, given size & value
            void assign(const std::size_t xi_count, const T& xi_value) {
                // allocate
                if (xi_count > m_reservedSize) {
                    m_reservedSize = 2 * xi_count;
                    reallocate();
                }
                m_size = xi_count;

                // fill
                for (std::size_t i{}; i < xi_count; ++i) {
                    m_data[i] = xi_value;
                }
            }

            // assigns new contents to the vector, given start/end iterators
            void assign(const T* xi_first, const T* xi_last) {
                // allocate
                const std::size_t count{ xi_last - xi_first };
                if (count > m_reservedSize) {
                    m_reservedSize = count << 2;
                    reallocate();
                }
                m_size = count;

                // fill
                for (std::size_t i{}; i < count; ++i, ++xi_first) {
                    m_data[i] = *xi_first;
                }
            }

            // assigns new contents to the vector, given initializer list
            void assign(std::initializer_list<T> xi_list) {
                // allocate
                const std::size_t count{ xi_list.size() };
                if (count > m_reservedSize) {
                    m_reservedSize = count << 2;
                    reallocate();
                }

                // fill
                std::size_t i{};
                for (auto& item : xi_list) {
                    m_data[i++] = item;
                }
            }

        // iterators
        public:
            
                  T* begin()        noexcept { return m_data; }
            const T* cbegin() const noexcept { return m_data; }

                  T* end()        noexcept { return m_data + m_size; }
            const T* cend() const noexcept { return m_data + m_size; }

            reverse_iterator rbegin() noexcept { return reverse_iterator(m_data + m_size); }
            const_reverse_iterator crbegin() const noexcept { return reverse_iterator(m_data + m_size); }

            reverse_iterator rend() noexcept { return reverse_iterator(m_data); }
            const_reverse_iterator crend() const noexcept { return reverse_iterator(m_data); }

        // capacity queries/modifiers
        public:

            // is vector empty?
            constexpr bool empty() const noexcept {
                return (m_size == 0);
            }

            // return vector size
            constexpr std::size_t size() const noexcept {
                return m_size;
            }

            // return vector maximum possible size
            constexpr std::size_t maxPossibleSize() const noexcept {
                return MaximumPossibleSize;
            }

            // return vector capacity
            constexpr std::size_t capacity() const noexcept {
                return m_reservedSize;
            }

            // resize vector to a given size
            void resize(const std::size_t xi_size) {
                if (xi_size > m_size) {
                    // allocate
                    if (xi_size > m_reservedSize) {
                        m_reservedSize = xi_size;
                        reallocate();
                    }
                }
                else {
                    // destroy excess elements
                    if constexpr (!std::is_arithmetic<T>::value) {
                        for (std::size_t i{ m_size }; i < xi_size; ++i) {
                            m_data[i].~T();
                        }
                    }
                }

                m_size = xi_size;
            }

            // resize vector to a given size and fill it with a given value
            void resize(const std::size_t xi_size, const T& xi_value) {
                if (xi_size > m_size) {
                    // allocate
                    if (xi_size > m_reservedSize) {
                        m_reservedSize = xi_size;
                        reallocate();
                    }
                    for (std::size_t i{ m_size }; i < xi_size; ++i)
                        m_data[i] = xi_value;
                }
                else {
                    // destroy excess elements
                    if constexpr (!std::is_arithmetic<T>::value) {
                        for (std::size_t i{ m_size }; i < xi_size; ++i) {
                            m_data[i].~T();
                        }
                    }
                }

                m_size = xi_size;
            }

            // reserve a given size
            void reserve(const std::size_t xi_size) {
                // allocate
                if (xi_size > m_reservedSize) {
                    m_reservedSize = xi_size;
                    reallocate();
                }
            }

            // shrink vector to its current size
            void shrink_to_fit() {
                m_reservedSize = m_size;
                reallocate();
            }

        // element wise access operations
        public:

            // [] operator overload
                  T& operator [](std::size_t idx)       { return m_data[idx]; }
            const T& operator [](std::size_t idx) const { return m_data[idx]; }

            // like [] but with exceptions
            T& at(std::size_t pos) {
                if (pos < m_size) {
                    return m_data[pos];
                } 
                else {
                    throw std::out_of_range("accessed position is out of range.");
                }
            }

            const T& at(std::size_t pos) const {
                if (pos < m_size) {
                    return m_data[pos];
                } 
                else {
                    throw std::out_of_range("accessed position is out of range");
                }
            }

            // return first element
                  T& front()       { return m_data[0]; }
            const T& front() const { return m_data[0]; }

            // return last element
                  T& back()       { return m_data[m_size - 1]; }
            const T& back() const { return m_data[m_size - 1]; };

        // underlying data access
        public:
                  T* data()       noexcept { return m_data; }
            const T* data() const noexcept { return m_data; }

        // general modifiers
        public:
            
            // emplace elements to vector head
            template <class... Args> void emplace_back(Args&& ... args) {
                if (m_size == m_reservedSize) {
                    m_reservedSize *= 2;
                    reallocate();
                }
                m_data[m_size] = std::move(T(std::forward<Args>(args) ...));
                ++m_size;
            }

            // push element to vector head
            void push_back(const T& xi_value) {
                if (m_size == m_reservedSize) {
                    m_reservedSize *= 2;
                    reallocate();
                }
                m_data[m_size] = xi_value;
                ++m_size;
            }

            void push_back(T&& xi_value) {
                if (m_size == m_reservedSize) {
                    m_reservedSize *= 2;
                    reallocate();
                }
                m_data[m_size] = std::move(xi_value);
                ++m_size;
            }

            // remove element from vector head
            void pop_back() {
                --m_size;
                m_data[m_size].~T();
            }

            // push elements to a vector from a given iterator, return iterator to last element 
            template <class ... Args> T* emplace(const T* xi_iterator, Args&& ... args) {
                iterator iit{ &m_data[xi_iterator - m_data] };
                if (m_size == m_reservedSize) {
                    m_reservedSize *= 2;
                    reallocate();
                }

                memmove(iit + 1, iit, (m_size - (xi_iterator - m_data)) * sizeof(T));
                (*iit) = std::move(T(std::forward<Args>(args) ...));
                ++m_size;

                return iit;
            }

            // insert an element to a vector from a given iterator, return iterator to element
            T* insert(const T* xi_iterator, const T& xi_value) {
                iterator iit{ &m_data[xi_iterator - m_data] };

                if (m_size == m_reservedSize) {
                    m_reservedSize *= 2;
                    reallocate();
                }

                memmove(iit + 1, iit, (m_size - (xi_iterator - m_data)) * sizeof(T));
                (*iit) = xi_value;
                ++m_size;

                return iit;
            }

            
            T* insert(const T* xi_iterator, T&& xi_value) {
                iterator iit{ &m_data[xi_iterator - m_data] };

                if (m_size == m_reservedSize) {
                    m_reservedSize *= 2;
                    reallocate();
                }

                memmove(iit + 1, iit, (m_size - (xi_iterator - m_data)) * sizeof(T));
                (*iit) = std::move(xi_value);
                ++m_size;

                return iit;
            }

            // insert an element a given number of times to a vector from a given iterator, return iterator to element
            T* insert(const T* xi_iterator, std::size_t xi_count, const T &xi_value) {
                iterator f{ &m_data[xi_iterator - m_data] };
                if (!xi_count) return f;

                if (m_size + xi_count > m_reservedSize) {
                    m_reservedSize = (m_size + xi_count) << 2;
                    reallocate();
                }

                memmove(f + xi_count, f, (m_size - (xi_iterator - m_data)) * sizeof(T));
                m_size += xi_count;

                for (iterator xi_iterator = f; xi_count--; ++xi_iterator) {
                    (*xi_iterator) = xi_value;
                }
                return f;
            }

            // insert an elements given by iterator range to a vector at a given iterator, return iterator to last element inserted
            template<class InputIt> T* insert(const T* xi_iterator, InputIt xi_first, InputIt xi_last) {
                iterator f{ &m_data[xi_iterator - m_data] };
                const std::size_t cnt{ xi_last - xi_first };
                if (!cnt) return f;

                if (m_size + cnt > m_reservedSize) {
                    m_reservedSize = (m_size + cnt) << 2;
                    reallocate();
                }

                memmove(f + cnt, f, (m_size - (xi_iterator - m_data)) * sizeof(T));
                for (iterator xi_iterator = f; xi_first != xi_last; ++xi_iterator, ++xi_first) {
                    (*xi_iterator) = *xi_first;
                }
                m_size += cnt;

                return f;
            }

            // insert a list to a vector at a given iterator, return iterator to last element inserted
            T* insert(const T* xi_iterator, std::initializer_list<T> xi_list) {
                const std::size_t cnt{ xi_list.size() };
                iterator f{ &m_data[xi_iterator - m_data] };
                if (!cnt) return f;

                if (m_size + cnt > m_reservedSize) {
                    m_reservedSize = (m_size + cnt) << 2;
                    reallocate();
                }

                memmove(f + cnt, f, (m_size - (xi_iterator - m_data)) * sizeof(T));
                iterator iit = f;
                for (auto &item : xi_list) {
                    (*iit) = item;
                    ++iit;
                }
                m_size += cnt;

                return f;
            }

            // erase element at a given iterator, return iterator to element found at new 'index'
            T* erase(const T* xi_iterator) {
                iterator iit{ &m_data[xi_iterator - m_data] };

                if constexpr (!std::is_arithmetic<T>::value) {
                    (*iit).~T();
                }

                memmove(iit, iit + 1, (m_size - (xi_iterator - m_data) - 1) * sizeof(T));
                --m_size;

                return iit;
            }

            // erase elements at a given range, return iterator to element found at new 'index'
            T* erase(const T* xi_first, const T* xi_last) {
                iterator f{ &m_data[xi_first - m_data] };
                if (xi_first == xi_last) return f;

                if constexpr (!std::is_arithmetic<T>::value) {
                    for (; xi_first != xi_last; ++xi_first) {
                        (*xi_first).~T();
                    }
                }

                memmove(f, xi_last, (m_size - (xi_last - m_data)) * sizeof(T));
                m_size -= xi_last - xi_first;

                return f;
            }

            // swap two vectors
            void swap(Vector<T> &rhs) {
                const size_t tvec_sz{ m_size },
                             trsrv_sz{ m_reservedSize };
                T *tarr = m_data;

                m_size = rhs.m_size;
                m_reservedSize = rhs.m_reservedSize;
                m_data = rhs.m_data;

                rhs.m_size = tvec_sz;
                rhs.m_reservedSize = trsrv_sz;
                rhs.m_data = tarr;
            }

            // clear a vector
            void clear() noexcept {
                if constexpr (!std::is_arithmetic<T>::value) {
                    for (std::size_t i{}; i < m_size; ++i) {
                        m_data[i].~T();
                    }
                }
                m_size = 0;
            }

        // 'numerical'/logical/bitwise operator overload
        public:

            // '+'/'+=' overload 
            template<typename RightExpr> Vector& operator +=(RightExpr&& xi_expression) {
                for (std::size_t i{}; i < m_size; ++i) {
                    m_data[i] += xi_expression[i];
                }
                return *this;
            }
            template<typename RightExpr> auto operator +(RightExpr&& xi_expression) const -> BinaryExpression<const Vector&, BinaryOperations::ADD<T>, decltype(std::forward<RightExpr>(xi_expression))> {
                return BinaryExpression<const Vector&, BinaryOperations::ADD<T>, decltype(std::forward<RightExpr>(xi_expression))>(*this, std::forward<RightExpr>(xi_expression));
            }

            // '-'/'-=' overload 
            template<typename RightExpr> Vector& operator -=(RightExpr&& xi_expression) {
                for (std::size_t i{}; i < m_size; ++i) {
                        m_data[i] -= xi_expression[i];
                }
                return *this;
            }
            template<typename RightExpr> auto operator -(RightExpr&& xi_expression) const -> BinaryExpression<const Vector&, BinaryOperations::SUB<T>, decltype(std::forward<RightExpr>(xi_expression))> {
                return BinaryExpression<const Vector&, BinaryOperations::SUB<T>, decltype(std::forward<RightExpr>(xi_expression))>(*this, std::forward<RightExpr>(xi_expression));
            }

            // '*'/'*=' overload 
            template<typename RightExpr> Vector& operator *=(RightExpr&& xi_expression) {
                for (std::size_t i{}; i < m_size; ++i) {
                    m_data[i] *= xi_expression[i];
                }
                return *this;
            }
            template<typename RightExpr> auto operator *(RightExpr&& xi_expression) const -> BinaryExpression<const Vector&, BinaryOperations::MUL<T>, decltype(std::forward<RightExpr>(xi_expression))> {
                return BinaryExpression<const Vector&, BinaryOperations::MUL<T>, decltype(std::forward<RightExpr>(xi_expression))>(*this, std::forward<RightExpr>(xi_expression));
            }

            // '/'/'/=' overload 
            template<typename RightExpr> Vector& operator /=(RightExpr&& xi_expression) {
                for (std::size_t i{}; i < m_size; ++i) {
                    m_data[i] /= xi_expression[i];
                }
                return *this;
            }
            template<typename RightExpr> auto operator /(RightExpr&& xi_expression) const -> BinaryExpression<const Vector&, BinaryOperations::DIV<T>, decltype(std::forward<RightExpr>(xi_expression))> {
                return BinaryExpression<const Vector&, BinaryOperations::DIV<T>, decltype(std::forward<RightExpr>(xi_expression))>(*this, std::forward<RightExpr>(xi_expression));
            }

            // '&'/'&=' overload 
            template<typename RightExpr> Vector& operator &=(RightExpr&& xi_expression) {
                for (std::size_t i{}; i < m_size; ++i) {
                    m_data[i] &= xi_expression[i];
                }
                return *this;
            }
            template<typename RightExpr> auto operator &(RightExpr&& xi_expression) const -> BinaryExpression<const Vector&, BinaryOperations::LAND<T>, decltype(std::forward<RightExpr>(xi_expression))> {
                return BinaryExpression<const Vector&, BinaryOperations::LAND<T>, decltype(std::forward<RightExpr>(xi_expression))>(*this, std::forward<RightExpr>(xi_expression));
            }

            // '|'/'|=' overload 
            template<typename RightExpr> Vector& operator |=(RightExpr&& xi_expression) {
                for (std::size_t i{}; i < m_size; ++i) {
                    m_data[i] |= xi_expression[i];
                }
                return *this;
            }
            template<typename RightExpr> auto operator &(RightExpr&& xi_expression) const -> BinaryExpression<const Vector&, BinaryOperations::LOR<T>, decltype(std::forward<RightExpr>(xi_expression))> {
                return BinaryExpression<const Vector&, BinaryOperations::LOR<T>, decltype(std::forward<RightExpr>(xi_expression))>(*this, std::forward<RightExpr>(xi_expression));
            }

            // '^'/'^=' overload 
            template<typename RightExpr> Vector& operator ^=(RightExpr&& xi_expression) {
                for (std::size_t i{}; i < m_size; ++i) {
                    m_data[i] ^= xi_expression[i];
                }
                return *this;
            }
            template<typename RightExpr> auto operator ^(RightExpr&& xi_expression) const -> BinaryExpression<const Vector&, BinaryOperations::LXOR<T>, decltype(std::forward<RightExpr>(xi_expression))> {
                return BinaryExpression<const Vector&, BinaryOperations::LXOR<T>, decltype(std::forward<RightExpr>(xi_expression))>(*this, std::forward<RightExpr>(xi_expression));
            }

            // '<<'/'<<=' overload 
            template<typename RightExpr> Vector& operator <<=(RightExpr&& xi_expression) {
                for (std::size_t i{}; i < m_size; ++i) {
                    m_data[i] <<= xi_expression[i];
                }
                return *this;
            }
            template<typename RightExpr> auto operator <<(RightExpr&& xi_expression) const -> BinaryExpression<const Vector&, BinaryOperations::SHL<T>, decltype(std::forward<RightExpr>(xi_expression))> {
                return BinaryExpression<const Vector&, BinaryOperations::SHL<T>, decltype(std::forward<RightExpr>(xi_expression))>(*this, std::forward<RightExpr>(xi_expression));
            }

            // '>>'/'>>=' overload 
            template<typename RightExpr> Vector& operator >>=(RightExpr&& xi_expression) {
                for (std::size_t i{}; i < m_size; ++i) {
                    m_data[i] >>= xi_expression[i];
                }
                return *this;
            }
            template<typename RightExpr> auto operator >>(RightExpr&& xi_expression) const -> BinaryExpression<const Vector&, BinaryOperations::SHR<T>, decltype(std::forward<RightExpr>(xi_expression))> {
                return BinaryExpression<const Vector&, BinaryOperations::SHR<T>, decltype(std::forward<RightExpr>(xi_expression))>(*this, std::forward<RightExpr>(xi_expression));
            }

        // relational operator overload
        public:

            // '=='
            template<typename RightExpr> auto operator ==(RightExpr&& xi_expression) const -> BinaryExpression<const Vector&, BinaryOperations::EQ<T>, decltype(std::forward<RightExpr>(xi_expression))> {
                return BinaryExpression<const Vector&, BinaryOperations::EQ<T>, decltype(std::forward<RightExpr>(xi_expression))>(*this, std::forward<RightExpr>(xi_expression));
            }

            // '!='
            template<typename RightExpr> auto operator !=(RightExpr&& xi_expression) const -> BinaryExpression<const Vector&, BinaryOperations::NEQ<T>, decltype(std::forward<RightExpr>(xi_expression))> {
                return BinaryExpression<const Vector&, BinaryOperations::NEQ<T>, decltype(std::forward<RightExpr>(xi_expression))>(*this, std::forward<RightExpr>(xi_expression));
            }

            // '<'
            template<typename RightExpr> auto operator <(RightExpr&& xi_expression) const -> BinaryExpression<const Vector&, BinaryOperations::LT<T>, decltype(std::forward<RightExpr>(xi_expression))> {
                return BinaryExpression<const Vector&, BinaryOperations::LT<T>, decltype(std::forward<RightExpr>(xi_expression))>(*this, std::forward<RightExpr>(xi_expression));
            }

            // '<='
            template<typename RightExpr> auto operator <=(RightExpr&& xi_expression) const -> BinaryExpression<const Vector&, BinaryOperations::LE<T>, decltype(std::forward<RightExpr>(xi_expression))> {
                return BinaryExpression<const Vector&, BinaryOperations::LE<T>, decltype(std::forward<RightExpr>(xi_expression))>(*this, std::forward<RightExpr>(xi_expression));
            }

            // '>'
            template<typename RightExpr> auto operator >(RightExpr&& xi_expression) const -> BinaryExpression<const Vector&, BinaryOperations::GT<T>, decltype(std::forward<RightExpr>(xi_expression))> {
                return BinaryExpression<const Vector&, BinaryOperations::GT<T>, decltype(std::forward<RightExpr>(xi_expression))>(*this, std::forward<RightExpr>(xi_expression));
            }

            // '>='
            template<typename RightExpr> auto operator >=(RightExpr&& xi_expression) const -> BinaryExpression<const Vector&, BinaryOperations::GE<T>, decltype(std::forward<RightExpr>(xi_expression))> {
                return BinaryExpression<const Vector&, BinaryOperations::GE<T>, decltype(std::forward<RightExpr>(xi_expression))>(*this, std::forward<RightExpr>(xi_expression));
            }
    };
};
