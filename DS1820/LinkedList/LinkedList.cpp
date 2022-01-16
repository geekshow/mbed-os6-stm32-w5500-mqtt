/**
 * @file    LinkedList.cpp
 * @brief   Core Utility - Templated Linked List class
 * @author  sam grove
 * @version 1.0
 * @see     
 *
 * Copyright (c) 2013
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "LinkedList.h"    // api wrapper

template<class retT>
LinkedList<retT>::LinkedList()
{
    // clear the member
    _head = 0;

    return;
}

template<class retT>
LinkedList<retT>::~LinkedList()
{
    // free any memory that is on the heap
    while(remove(1) != NULL);

    return;
}

template<class retT>
retT *LinkedList<retT>::push(void *data)
{
    retT *new_node = new retT [1];
    // make sure the new object was allocated
    if (0 == new_node)
    {
        error("Memory allocation failed\n");
    }
    // update the next item in the list to the current head
    new_node->next = _head;
    // store the address to the linked datatype
    new_node->data = data;
    _head = new_node;

    return _head;
}

//template<class retT>
//retT *LinkedList<retT>::insert(void *data, uint32_t loc)
//{
//    retT *new_node = new retT [1];
//    // make sure the new object was allocated
//    if (0 == new_node)
//    {
//        error("Memory allocation failed\n");
//    }
//    retT *current = _head->next;
//    retT *prev = _head;
//    // move to the item we want to insert
//    for (uint32_t i=1; i!=(loc-1); i++)
//    {
//        prev = current;
//        current = current->next;
//    }
//    // store the address to the linked datatype
//    new_node->data = data;
//    // clear the next pointer
//    new_node->next = 0;
//    // update the list and store the new stuff
//    prev->next = new_node;
//    new_node->next = current;
//    // store the address to the linked datatype
//    _head->data = data;
//
//    return prev->next;
//}

template<class retT>
retT *LinkedList<retT>::append(void *data)
{
    retT *current = _head;
    retT *new_node = new retT [1];
    // make sure the new object was allocated
    if (0 == new_node)
    {
        error("Memory allocation failed\n");
    }
    // store the address to the linked datatype
    new_node->data = data;
    // clear the next pointer
    new_node->next = 0;
    // check for an empty list
    if (0 == current)
    {
        _head = new_node;
        return _head;
    }
    else
    {
        // look for the end of the list
        while (current->next != 0)
        {
            current = current->next;
        }
        // and then add the new item to the list
        current->next = new_node;
    }

    return current->next;
}

template<class retT>
retT *LinkedList<retT>::remove(uint32_t loc)
{
    retT *current = _head;
    retT *prev = 0;
    // make sure we have an item to remove
    if ((loc <= length()) && (loc > 0))
    {
        // move to the item we want to delete
        if (1 == loc)
        {
            _head = current->next;
            delete [] current;
        }
        else
        {
            for (uint32_t i=2; i<=loc; ++i)
            {
                prev = current;
                current = current->next;
            }
            // store the item + 1 to replace what we are deleting
            prev->next = current->next;
            delete [] current;
        }
    }

    return _head;
}

template<class retT>
retT *LinkedList<retT>::pop(uint32_t loc)
{
    retT *current = _head;
    // make sure we have something in the location
    if ((loc > length()) || (loc == 0))
    {
        return 0;
    }
    // and if so jump down the list
    for (uint32_t i=2; i<=loc; ++i)
    {
        current = current->next;
    }

    return current;
}

template<class retT>
uint32_t LinkedList<retT>::length(void)
{
    int32_t count = 0;
    retT *current = _head;
    //loop until the end of the list is found
    while (current != 0)
    {
        ++count;
        current = current->next;
    }

    return count;
}

// pre-define the type for the linker
template class LinkedList<node>;





