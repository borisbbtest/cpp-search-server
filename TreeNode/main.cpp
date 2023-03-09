#include <cassert>

template <typename T>
struct TreeNode
{
    T value;
    TreeNode *left = nullptr;
    TreeNode *right = nullptr;
};

template <typename T>
void DeleteTree(TreeNode<T> *node)
{
    if (!node)
    {
        return;
    }
    DeleteTree(node->left);
    DeleteTree(node->right);
    delete node;
}
template <typename T>
bool CheckTreeProperty(const TreeNode<T> *node, const T *min, const T *max)
{
    if (node->value < *max && node->value > *min)
    {
        if(node->right != nullptr )
        {
            return CheckTreeProperty<T>(node, &node->left->value, &node->right->value);
        }
        if(node->left != nullptr)
        {
            return CheckTreeProperty<T>(node, nullptr, nullptr);
        }
    }
    else
    {
        return 0;
    }
    return 1;
}
template <typename T>
bool CheckTreeProperty(const TreeNode<T> *node)
{
    // реализуйте функцию
    return CheckTreeProperty<T>(node, nullptr, nullptr);
}

int main()
{
    using T = TreeNode<int>;
    T *root1 = new T{6,
                     new T{4, new T{3}, new T{5}}, new T{7}};
    assert(CheckTreeProperty(root1));

    T *root2 = new T{6,
                     new T{4, new T{3}, new T{5}}, new T{7, new T{8}}};
    assert(!CheckTreeProperty(root2));

    DeleteTree(root1);
    DeleteTree(root2);
}