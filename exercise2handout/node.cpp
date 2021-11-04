#include "Node.h"
#include "maths_funcs.h"

Node::Node() : parent(0)
{
}

void Node::init()
{
    position = vec3(0, 0, 0);
    rotation = versor(0, 0, 0, 1);
    scale = vec3(1, 1, 1);

    localMatrix = identity_mat4();
    worldMatrix = identity_mat4();
    localInverseMatrix = identity_mat4();
    worldInverseMatrix = identity_mat4();
}

void Node::addChild(Node &node)
{
    node.parent = this;
    children.push_back(&node);
}

void Node::removeChild(Node &node)
{
    auto it = std::find(children.begin(), children.end(), &node);
    if (it != children.end())
    {
        node.parent = nullptr;
        children.erase(it);
    }
}

// --------------------------------------------------------------------------- REVIEW
void Node::updateLocal()
{
    // 00, 04, 08, 12,
    // 01, 05, 09, 13,
    // 02, 06, 10, 14,
    // 03, 07, 11, 15,

    auto T = identity_mat4();
    auto S = identity_mat4();
    auto R = quat_to_mat4(rotation);

    T.m[12] = position.x;
    T.m[13] = position.y;
    T.m[14] = position.z;
    T.m[15] = 1.f;

    S.m[00] = scale.x;
    S.m[05] = scale.y;
    S.m[10] = scale.z;
    S.m[15] = 1.f;

    auto Tinv = translate(identity_mat4(), position * -1.f);
    auto Sinv = identity_mat4();
    auto Rinv = transpose(R);

    // FLT_EPSILON addition should prevent inf when scale is set to 0
    auto noZero = [](float &f) { return f == 0.f ? f + FLT_EPSILON : f; };
    Sinv.m[00] = 1 / noZero(scale.x);
    Sinv.m[05] = 1 / noZero(scale.y);
    Sinv.m[10] = 1 / noZero(scale.z);
    Sinv.m[15] = 1.f;

    auto debug = inverse(S) * inverse(R) * inverse(T);
    auto inv = Sinv * Rinv * Tinv;

    localMatrix = T * R * S;
    localInverseMatrix = inv;
}

void Node::updateHierarchy()
{
    updateLocal();

    // Set parent matrices as if they were 0,0
    mat4 parentMatrix = identity_mat4();
    mat4 parentInverseMatrix = identity_mat4();

    // Search parent matrices
    if (parent != nullptr)
    {
        parentMatrix = parent->worldMatrix;
        parentInverseMatrix = parent->worldInverseMatrix;
    }

    // Localize node space in parent space
    // worldMatrix is the addition of all node spaces up until this node
    worldMatrix = parentMatrix * localMatrix;
    worldInverseMatrix = parentInverseMatrix * localInverseMatrix;

    // Update children
    for (auto &child : children)
        if (child != nullptr)
            child->updateHierarchy();
}
// ---------------------------------------------------------------------------
