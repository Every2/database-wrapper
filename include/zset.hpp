#include "avl.hpp"
#include "common.hpp"
#include <unordered_map>

template<typename T, typename Y>
struct ZNode {
    AvlTree<ZNode> tree;
    std::unordered_map<T, Y> hash_map;
    double score {0};
    size_t len {0};
    std::string name;
};

template<typename T, typename Y>
class ZSet {

public:
    bool add(std::string& name, double score) {
        auto object {hash_map.find(name)};
        if (object != hash_map.end()) {
            update(object->second, score);
            return false;
        } else  {
            insert(name, score);
            return true;
        }
    }
    ZNode lookup(std::string& name) {
        auto object {hash_map.find(name)};
        return (object != hash_map.end()) ? object->second.get() : nullptr;
    }
    ZNode* pop(std::string& name) {
        auto object {hash_map.find(name)};
        if (object != hash_map.end()) {
            ZNode* node {object->second.get()};
            hash_map.erase(object);
            tree.remove(node);
            return node;
        }

        return nullptr;
    }

    ZNode* query(double score, const std::string& name, int64_t offset) {
        ZNode* result {nullptr};

        auto current {root.get()};

        while (current) {
            if (current->data.score == score) {
                int comparison {name.compare(current->data.name)};
                if (comparison < 0) {
                    result = current;
                    current = current->left.get();
                } else if (comparison > 0) {
                    current = current->right.get();
                } else {
                    result = current;
                    break;
                }
            } else if (current->data.score < score) {
                current = current->right.get();
            } else {
                result = current;
                current = current->left.get();
            }
        }


        while (result && offset > 0) {
            result = predecessor(result);
            offset--;
        }
        while (result && offset < 0) {
            result = successor(result);
            offset++;
        }

        return result;
    }   


    void update(const std::string& name, double score) {
        auto object {hash_map.find(name)};
        if (object != hash_map.end()) {
            ZNode* node {it->second.get()};
            tree.remove(node);
            node->score = score;
            tree.insert(node);
        }
    }

private:
    AvlTree<ZSet> tree;
    std::unordered_map<T, Y> hash_map;
    void insert(const std::string& name, double score) {
        auto node {std::make_unique<ZNode>(name, score)};
        hash_map.at(name) = std::move(node);
        tree.insert(hash_map.at(name).get());
    }
};