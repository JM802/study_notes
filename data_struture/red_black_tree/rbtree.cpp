#include <iostream>
#include <vector>
#include <string>
#include <cstdint>

using namespace std;

enum class rbcolor : uint8_t
{
    red,
    black
};

template <typename T>
class rbnodebase
{
public:
    T *right = nullptr;
    T *left = nullptr;
    T *parent = nullptr;
    rbcolor color = rbcolor::red;
    bool isred() const
    {
        return color == rbcolor::red;
    }
    bool isblack() const
    {
        return color == rbcolor::black;
    }

    void setred()
    {
        color = rbcolor::red;
    }

    void setblack()
    {
        color = rbcolor::black;
    }
};

template <typename key_type>
class rbtree_node : public rbnodebase<rbtree_node<key_type>>
{
public:
    key_type key;
    void *value = nullptr;
    rbtree_node(key_type k) : key(k)
    {
    }
};

template <typename key_type>
class rbtree
{
public:
    rbtree_node<key_type> *root;
    rbtree_node<key_type> *nil;
};

template <typename key_type>
void rbtree_left_rotate(rbtree_node<key_type> *x, rbtree<key_type> *T)
{
    rbtree_node<key_type> *y = x->right;
    rbtree_node<key_type> *old_y_left = y->left;
    rbtree_node<key_type> *old_x_parent = x->parent;

    // 必须先，否则x的父指针断裂无法判断x到底是root left right
    y->parent = old_x_parent;
    if (old_x_parent == T->nil)
    {
        T->root = y;
    } // x为根节点，父节点是虚拟的
    else if (x == old_x_parent->left)
    {
        old_x_parent->left = y;
    }
    else
    {
        old_x_parent->right = y;
    }

    x->parent = y;
    y->left = x;

    x->right = old_y_left;
    if (old_y_left != T->nil)
    {
        old_y_left->parent = x;
    } // nil是虚拟节点没有parent left right
}

template <typename key_type>
void rbtree_right_rotate(rbtree_node<key_type> *y, rbtree<key_type> *T)
{
    rbtree_node<key_type> *x = y->left;
    rbtree_node<key_type> *old_y_parent = y->parent;
    rbtree_node<key_type> *old_x_right = x->right;

    x->parent = old_y_parent;
    if (old_y_parent == T->nil)
    {
        T->root = x;
    }
    else if (old_y_parent->left == y)
    {
        old_y_parent->left = x;
    }
    else
    {
        old_y_parent->right = x;
    }

    y->parent = x;
    x->right = y;

    y->left = old_x_right;
    if (old_x_right != T->nil)
    {
        old_x_right->parent = y;
    }
}