#include <algorithm>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

//Original implementation https://gist.github.com/phoemur/6dd18d608438373185f6a2457662c1c2
template<typename T>
class AvlTree {
    struct Node {
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;
        T data;
        std::int32_t height;

        template<typename X = T>
        Node(X&& ele,
             std::unique_ptr<Node>&& lt,
             std::unique_ptr<Node>&& rt,
             std::int32_t h = 0)
                : left{std::move(lt)},
                  right{std::move(rt)},
                  data{std::forward<X>(ele)},
                  height{h} {}
    };

    std::unique_ptr<Node> root;
    std::size_t sz;
    
public:
    AvlTree() : root{nullptr}, sz{0} {}
    AvlTree(const AvlTree& other)
        : root{clone(other.root)}, sz{other.sz} {}
        
    AvlTree& operator=(const AvlTree& other) 
    {
        AvlTree tmp (other);
        std::swap(root, tmp.root);
        std::swap(sz, tmp.sz);

        return *this;
    }
    
    AvlTree(AvlTree&& other) noexcept
        : AvlTree()
    {
        std::swap(root, other.root);
        std::swap(sz, other.sz);
    }
    
    AvlTree& operator=(AvlTree&& other) noexcept
    {
        std::swap(root, other.root);
        std::swap(sz, other.sz);
        return *this;
    }
    
    ~AvlTree() noexcept = default;

    template<typename Iter>
    AvlTree(Iter first, Iter last)
        : AvlTree()
    {
        using c_tp = typename std::iterator_traits<Iter>::value_type;
        static_assert(std::is_constructible<T, c_tp>::value, "Type mismatch");

        for (auto it = first; it != last; ++it) {
            insert(*it);
        }
    }

    AvlTree(const std::initializer_list<T>& lst)
        : AvlTree(std::begin(lst), std::end(lst)) {}

    AvlTree(std::initializer_list<T>&& lst)
        : AvlTree()
    {
        for (auto&& elem: lst) {
            insert(std::move(elem));
        }    
    }

    AvlTree& operator=(const std::initializer_list<T>& lst)
    {
        AvlTree tmp (lst);
        std::swap(root, tmp.root);
        std::swap(sz, tmp.sz);

        return *this;
    }
    
    inline bool empty() const noexcept
    {
        return sz == 0;
    }
    
    inline const std::size_t& size() const noexcept
    {
        return sz;
    }
    
    template<typename X = T,
             typename... Args>
    void insert(X&& first, Args&&... args)
    {
        insert_util(std::forward<X>(first), root);
        ++sz;
        insert(std::forward<Args>(args)...);
    }
    
    template<typename X = T>
    void insert(X&& first)
    {
        insert_util(std::forward<X>(first), root);
        ++sz;
    }
    
    template<typename X = T,
             typename... Args>
    void remove(const X& first, Args&&... args) noexcept
    {
        remove_util(first, root);
        --sz;
        remove(std::forward<Args>(args)...);
    }
    
    void remove(const T& first) noexcept
    {
        remove_util(first, root);
        --sz;
    }
    
    const T& min_element() const
    {
        if (empty()) throw std::logic_error("Empty container");
        return findMin(root);
    }
    
    const T& max_element() const
    {
        if (empty()) throw std::logic_error("Empty container");
        return findMax(root);
    }
    
    void clear() noexcept
    {
        root.reset(nullptr);
    }
    
    void print() const noexcept
    {
        if (empty()) std::cout << "{}\n";
        else {
            std::cout << "{";
            print(root);
            std::cout << "\b\b}\n";
        }
    }
    
    bool search(const T& x) const noexcept
    {
        return search(x, root);
    }
        
private:
    std::unique_ptr<Node> clone(const std::unique_ptr<Node>& node) const
    {
        if (!node) return nullptr;
        else
            return std::make_unique<Node>(node->data, 
                                          clone(node->left), 
                                          clone(node->right), 
                                          node->height);
    }
    
    inline std::int32_t height(const std::unique_ptr<Node>& node) const noexcept
    {
        return node == nullptr ? -1 : node->height;
    }
    
    void print(const std::unique_ptr<Node>& t) const noexcept
    { 
        if (t != nullptr) {
            print(t->left);
            std::cout << t->data << ", ";
            print(t->right);
        }
    }
    
    bool search(const T& x, const std::unique_ptr<Node>& node) const noexcept
    {
        auto t = node.get();
        
        while(t != nullptr)
            if(x < t->data)
                t = t->left.get();
            else if(t->data < x)
                t = t->right.get();
            else
                return true;

        return false;
    }
    
    template<typename X = T>
    void insert_util(X&& x, std::unique_ptr<Node>& t)
    {
        if (t == nullptr)
            t = std::make_unique<Node>(std::forward<X>(x), nullptr, nullptr);
        else if (x < t->data)
            insert_util(std::forward<X>(x), t->left);
        else if (t->data < x)
            insert_util(std::forward<X>(x), t->right);
            
        balance(t);
    }
    
    void remove_util(const T& x, std::unique_ptr<Node>& t) noexcept
    {
        if(t == nullptr) return;   
        
        if(x < t->data)
            remove_util(x, t->left);
        else if(t->data < x)
            remove_util(x, t->right);
        else if(t->left != nullptr && t->right != nullptr) { 
            t->data = findMin(t->right);
            remove_util(t->data, t->right);
        }
        else { 
            std::unique_ptr<Node> oldNode {std::move(t)};
            t = (oldNode->left != nullptr) ? std::move(oldNode->left) : 
                                             std::move(oldNode->right);
        }
        balance(t);
    }
    
    const T& findMin(const std::unique_ptr<Node>& node) const noexcept
    {
        auto t = node.get();

        if(t != nullptr)
            while (t->left != nullptr)
                t = t->left.get();

        return t->data;
    }

    const T& findMax(const std::unique_ptr<Node>& node) const noexcept
    {
        auto t = node.get();

        if(t != nullptr)
            while (t->right != nullptr)
                t = t->right.get();

        return t->data;
    }

    void balance(std::unique_ptr<Node>& t) noexcept
    {
        static const int ALLOWED_IMBALANCE = 1;
        
        if(t == nullptr) return;
        
        if(height(t->left) - height(t->right) > ALLOWED_IMBALANCE) {
            if(height(t->left->left) >= height(t->left->right))
                rotateWithLeftChild(t);
            else
                doubleWithLeftChild(t);
        }
        else if(height(t->right) - height(t->left) > ALLOWED_IMBALANCE) {
            if(height(t->right->right) >= height(t->right->left))
                rotateWithRightChild(t);
            else
                doubleWithRightChild(t);
        }
        
        t->height = std::max(height(t->left), height(t->right)) + 1;
    }
    
    void rotateWithLeftChild(std::unique_ptr<Node>& k2) noexcept
    {
        auto k1 = std::move(k2->left);
        k2->left = std::move(k1->right);
        k2->height = std::max(height(k2->left), height(k2->right)) + 1;
        k1->height = std::max(height(k1->left), k2->height) + 1;
        k1->right = std::move(k2);
        k2 = std::move(k1);
    }
    

    void rotateWithRightChild(std::unique_ptr<Node>& k1) noexcept
    {
        auto k2 = std::move(k1->right);
        k1->right = std::move(k2->left);
        k1->height = std::max(height(k1->left), height(k1->right)) + 1;
        k2->height = std::max(height(k2->right), k1->height) + 1;
        k2->left = std::move(k1);
        k1 = std::move(k2);
    }
    
    void doubleWithLeftChild(std::unique_ptr<Node>& k3) noexcept
    {
        rotateWithRightChild(k3->left);
        rotateWithLeftChild(k3);
    }

    void doubleWithRightChild(std::unique_ptr<Node>& k1) noexcept
    {
        rotateWithLeftChild(k1->right);
        rotateWithRightChild(k1);
    }
    
}; 