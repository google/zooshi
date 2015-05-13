// Copyright 2015 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef INTRUSIVE_LIST_H_
#define INTRUSIVE_LIST_H_

#include <cassert>
#include <cstdint>
#include "fplbase/fpl_common.h"

namespace fpl {
// Whether to enable IntrusiveList::ValidateList().  Be careful when enabling
// this since this changes the size of IntrusiveListNode so make sure *all*
// projects that include intrusive_list.h also define this value in the same way
// to avoid data corruption.
#ifndef INTRUSIVE_LIST_VALIDATE
#define INTRUSIVE_LIST_VALIDATE 0
#endif  // INTRUSIVE_LIST_VALIDATE

// IntrusiveListNode is used to implement an intrusive doubly-linked
// list.
//
// For example:
//
// class MyClass {
// public:
//   MyClass(const char *msg) : msg_(msg) {}
//   const char* GetMessage() const { return msg_; }
//   INTRUSIVE_GET_NODE(node_);
//   INTRUSIVE_LIST_NODE_GET_CLASS(MyClass, node_);
// private:
//   IntrusiveListNode node_;
//   const char *msg_;
// };
//
// int main(int argc, char *argv[]) {
//   IntrusiveListNode list; // NOTE: type is NOT MyClass
//   MyClass a("this");
//   MyClass b("is");
//   MyClass c("a");
//   MyClass d("test");
//   list.InsertBefore(a.GetListNode());
//   list.InsertBefore(b.GetListNode());
//   list.InsertBefore(c.GetListNode());
//   list.InsertBefore(d.GetListNode());
//   for (IntrusiveListNode* node = list.GetNext();
//        node != list.GetTerminator(); node = node->GetNext()) {
//     MyClass *cls = MyClass::GetInstanceFromListNode(node);
//     printf("%s\n", cls->GetMessage());
//   }
//   return 0;
// }
class IntrusiveListNode {
 public:
  // Initialize the node.
  IntrusiveListNode() {
    Initialize();
#if INTRUSIVE_LIST_VALIDATE
    magic_ = k_magic;
#endif  // INTRUSIVE_LIST_VALIDATE
  }

  IntrusiveListNode(IntrusiveListNode&& node) {
    if (node.InList()) {
      next_ = node.next_;
      prev_ = node.prev_;
      node.next_->prev_ = this;
      node.prev_->next_ = this;
    } else {
      // If the other node was not in a list (i.e. pointing to itself), this
      // node should point to itself as well.
      Initialize();
    }
  }

  // If the node is in a list, remove it from the list.
  ~IntrusiveListNode() {
    Remove();
#if INTRUSIVE_LIST_VALIDATE
    magic_ = 0;
#endif  // INTRUSIVE_LIST_VALIDATE
  }

  // Insert this node after the specified node.
  void InsertAfter(IntrusiveListNode* const node) {
    assert(!node->InList());
    node->next_ = next_;
    node->prev_ = this;
    next_->prev_ = node;
    next_ = node;
  }

  // Insert this node before the specified node.
  void InsertBefore(IntrusiveListNode* const node) {
    assert(!node->InList());
    node->next_ = this;
    node->prev_ = prev_;
    prev_->next_ = node;
    prev_ = node;
  }

  template <class Comparitor>
  void Sort(Comparitor comparitor) {
    // Sort using insertion sort.
    // http://en.wikipedia.org/wiki/Insertion_sort
    IntrusiveListNode* next;
    for (IntrusiveListNode* i = GetNext()->GetNext(); i != GetTerminator();
         i = next) {
      // Cache the `next` node because `i` might move.
      next = i->GetNext();
      IntrusiveListNode* j = i;
      while (j != GetNext() && comparitor(*i, *j->GetPrevious())) {
        j = j->GetPrevious();
      }
      if (i != j) {
        j->InsertBefore(i->Remove());
      }
    }
  }

  // Get the terminator of the list.
  const IntrusiveListNode* GetTerminator() const { return this; }

  // Remove this node from the list it's currently in.
  IntrusiveListNode* Remove() {
    prev_->next_ = next_;
    next_->prev_ = prev_;
    Initialize();
    return this;
  }

  // Determine whether this list is empty or the node isn't in a list.
  bool IsEmpty() const { return GetNext() == this; }

  // Determine whether this node is in a list or the list contains nodes.
  bool InList() const { return !IsEmpty(); }

  // Calculate the length of the list.
  std::uint32_t GetLength() const {
    std::uint32_t length = 0;
    const IntrusiveListNode* const terminator = GetTerminator();
    for (const IntrusiveListNode* node = GetNext(); node != terminator;
         node = node->GetNext()) {
      length++;
    }
    return length;
  }

  // Get the next node in the list.
  IntrusiveListNode* GetNext() const { return next_; }

  // Get the previous node in the list.
  IntrusiveListNode* GetPrevious() const { return prev_; }

  // If INTRUSIVE_LIST_VALIDATE is 1 perform a very rough validation
  // of all nodes in the list.
  bool ValidateList() const {
#if INTRUSIVE_LIST_VALIDATE
    if (magic_ != k_magic) return false;
    const IntrusiveListNode* const terminator = GetTerminator();
    for (IntrusiveListNode* node = GetNext(); node != terminator;
         node = node->GetNext()) {
      if (node->magic_ != k_magic) return false;
    }
#endif  // INTRUSIVE_LIST_VALIDATE
    return true;
  }

  // Determine whether the specified node is present in this list.
  bool FindNodeInList(IntrusiveListNode* const node_to_find) const {
    const IntrusiveListNode* const terminator = GetTerminator();
    for (IntrusiveListNode* node = GetNext(); node != terminator;
         node = node->GetNext()) {
      if (node_to_find == node) return true;
    }
    return false;
  }

  // Initialize the list node.
  void Initialize() {
    next_ = this;
    prev_ = this;
  }

 private:
  FPL_DISALLOW_COPY_AND_ASSIGN(IntrusiveListNode);

#if INTRUSIVE_LIST_VALIDATE
  std::uint32_t magic_;
#endif  // INTRUSIVE_LIST_VALIDATE
  // The next node in the list.
  IntrusiveListNode* prev_;
  // The previous node in the list.
  IntrusiveListNode* next_;

#if INTRUSIVE_LIST_VALIDATE
  static const std::uint32_t k_magic = 0x7157ac01;
#endif  // INTRUSIVE_LIST_VALIDATE
};

typedef IntrusiveListNode IntrusiveList;

// Declares a member function to retrieve a pointer to NodeMemberName.
#define INTRUSIVE_GET_NODE_ACCESSOR(NodeMemberName, FunctionName) \
  IntrusiveListNode* FunctionName() { return &NodeMemberName; }   \
  const IntrusiveListNode* FunctionName() const { return &NodeMemberName; }

// Declares a member function GetListNode() to retrieve a pointer to
// NodeMemberName.
#define INTRUSIVE_GET_NODE(NodeMemberName) \
  INTRUSIVE_GET_NODE_ACCESSOR(NodeMemberName, GetListNode)

// Declares the member function FunctionName of Class to retrieve a pointer
// to a Class instance from a list node pointer.  NodeMemberName references
// the name of the IntrusiveListNode member of Class.
#define INTRUSIVE_LIST_NODE_GET_CLASS_ACCESSOR(Class, NodeMemberName, \
                                               FunctionName)          \
  static Class* FunctionName(IntrusiveListNode* node) {               \
    Class* cls = nullptr;                                             \
    /* This effectively performs offsetof(Class, NodeMemberName) */   \
    /* which ends up in the undefined behavior realm of C++ but in */ \
    /* practice this works with most compilers. */                    \
    return reinterpret_cast<Class*>(                                  \
        reinterpret_cast<std::uint8_t*>(node) -                       \
        reinterpret_cast<std::uint8_t*>(&cls->NodeMemberName));       \
  }                                                                   \
                                                                      \
  static const Class* FunctionName(const IntrusiveListNode* node) {   \
    return FunctionName(const_cast<IntrusiveListNode*>(node));        \
  }

// Declares the member function GetInstanceFromListNode() of Class to retrieve
// a pointer to a Class instance from a list node pointer.  NodeMemberName
// reference the name of the IntrusiveListNode member of Class.
#define INTRUSIVE_LIST_NODE_GET_CLASS(Class, NodeMemberName)    \
  INTRUSIVE_LIST_NODE_GET_CLASS_ACCESSOR(Class, NodeMemberName, \
                                         GetInstanceFromListNode)

// Declares the macro to iterate over a typed intrusive list.
#define INTRUSIVE_LIST_NODE_ITERATOR(type, listptr, iteidentifier_, statement) \
  {                                                                            \
    type* terminator = (listptr).GetTerminator();                              \
    for (type* iteidentifier_ = (listptr).GetNext();                           \
         iteidentifier_ != terminator;                                         \
         iteidentifier_ = iteidentifier_->GetNext()) {                         \
      statement;                                                               \
    }                                                                          \
  }

}  // namespace fpl

#endif  // INTRUSIVE_LIST_H_

