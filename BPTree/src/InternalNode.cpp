#include "InternalNode.hpp"

//creates internal node pointed to by tree_ptr or creates a new one
InternalNode::InternalNode(const TreePtr &tree_ptr) : TreeNode(INTERNAL, tree_ptr) {
    this->keys.clear();
    this->tree_pointers.clear();
    if(!is_null(tree_ptr))
        this->load();
}

//max element from tree rooted at this node
Key InternalNode::max() {
    Key max_key = DELETE_MARKER;
    TreeNode* last_tree_node = TreeNode::tree_node_factory(this->tree_pointers[this->size - 1]);
    max_key = last_tree_node->max();
    delete last_tree_node;
    return max_key;
}

//if internal node contains a single child, it is returned
TreePtr InternalNode::single_child_ptr() {
    if(this->size == 1)
        return this->tree_pointers[0];
    return NULL_PTR;
}

//inserts <key, record_ptr> into subtree rooted at this node.
//returns pointer to split node if exists
TreePtr InternalNode::insert_key(const Key &key, const RecordPtr &record_ptr) {
    TreePtr new_tree_ptr = NULL_PTR;

    bool inserted = false;
    // Handle case where node lies in middle
    for (uint i = 0 ; i + 1 < this->size ; i++ ) {
        if (key <= this->keys[i]) {
            auto* next_node = TreeNode::tree_node_factory(this->tree_pointers[i]);
            new_tree_ptr = next_node->insert_key(key, record_ptr);

            this->keys[i] = next_node->max();
            // Handle split
            if (new_tree_ptr != NULL_PTR) {
                auto* split_node = TreeNode::tree_node_factory(new_tree_ptr);
                this->keys.insert(this->keys.begin() + i + 1, split_node->max());
                this->tree_pointers.insert(this->tree_pointers.begin() + i + 1, new_tree_ptr);
                this->size ++ ;
                delete split_node;
            }
            inserted = true;
            delete next_node;
            break;
        }
    }

    // Handle case where node lies at the end
    if (!inserted) {
        auto* next_node = TreeNode::tree_node_factory(this->tree_pointers[this->size - 1]);
        new_tree_ptr = next_node->insert_key(key, record_ptr);

        // Handle split
        if (new_tree_ptr != NULL_PTR) {
            this->keys.push_back(next_node->max());
            this->tree_pointers.push_back(new_tree_ptr);
            this->size ++ ;
        }
        delete next_node;
    }

    new_tree_ptr = NULL_PTR;
    // Handle overflow
    if (this->overflows()) {
        int deleteFrom = MIN_OCCUPANCY;
        auto split_node = new InternalNode();
        new_tree_ptr = split_node->tree_ptr;

        // transfer data to split_node
        for (uint it = deleteFrom ; it < this->size ; it++ ) {
            split_node->tree_pointers.push_back(this->tree_pointers[it]);
            if (it + 1 != this->size) {
                split_node->keys.push_back(this->keys[it]);
            }
        }

        // remove transfered data from internal node
        split_node->size = split_node->tree_pointers.size();
        this->keys.resize(this->keys.size() - (this->size - deleteFrom));
        this->tree_pointers.resize(this->tree_pointers.size() - (this->size - deleteFrom));
        this->size = this->tree_pointers.size();
        split_node->dump();
        delete split_node;
    }

    this->dump();
    return new_tree_ptr;
}

//redistributes 2 nodes at position 'position - 1' & 'position'
void InternalNode::redistribute_nodes(const uint position) {
    auto* next_node = TreeNode::tree_node_factory(this->tree_pointers[position]);
    // nodes are leaf nodes
    if (next_node->node_type == LEAF) {
        auto* previous_node_leaf = new LeafNode(this->tree_pointers[position - 1]);
        auto* next_node_leaf = new LeafNode(this->tree_pointers[position]);

        auto all_pointers = previous_node_leaf->data_pointers;

        for (auto it : all_pointers)
            previous_node_leaf->delete_key(it.first);

        auto next_node_pointers = next_node_leaf->data_pointers;
        all_pointers.insert(next_node_pointers.begin(), next_node_pointers.end());
        for (auto it : next_node_pointers)
            next_node_leaf->delete_key(it.first);

        uint c = 0;
        for (auto it = all_pointers.rbegin() ; it != all_pointers.rend() ; it ++, c ++ ) {
            if (c < MIN_OCCUPANCY)
                next_node_leaf->insert_key(it->first, it->second);
            else
                previous_node_leaf->insert_key(it->first, it->second);
        }

        this->keys[position - 1] = previous_node_leaf->max();
        if (position + 1 != this->size)
            this->keys[position] = next_node_leaf->max();

        previous_node_leaf->dump();
        next_node_leaf->dump();

        all_pointers.clear();
        next_node_pointers.clear();
        delete previous_node_leaf;
        delete next_node_leaf;

    } else { // nodes are internal nodes
        auto* previous_node_internal = new InternalNode(this->tree_pointers[position - 1]);
        auto* next_node_internal = new InternalNode(this->tree_pointers[position]);

        auto all_pointers = previous_node_internal->tree_pointers;
        all_pointers.insert(all_pointers.end(), next_node_internal->tree_pointers.begin(), next_node_internal->tree_pointers.end());

        previous_node_internal->tree_ptr.clear();
        next_node_internal->tree_ptr.clear();
        previous_node_internal->size = next_node_internal->size = 0;

        uint c = 0;
        for (uint it = all_pointers.size() - 1 ; it >= 0 ; it ++, c ++ ) {
            if (c < MIN_OCCUPANCY) {
                next_node_internal->tree_pointers.push_back(all_pointers[it]);
                next_node_internal->size ++ ;
            } else {
                previous_node_internal->tree_pointers.push_back(all_pointers[it]);
                previous_node_internal->size ++ ;                                
            }
        }

        this->keys[position - 1] = previous_node_internal->max();
        if (position + 1 != this->size)
            this->keys[position] = next_node_internal->max();

        previous_node_internal->dump();
        next_node_internal->dump();

        all_pointers.clear();
        delete previous_node_internal;
        delete next_node_internal;
    }

    delete next_node;
}

//merges 2 nodes at position 'position - 1' & 'position'
void InternalNode::merge_nodes(const uint position) {
    auto* next_node = TreeNode::tree_node_factory(this->tree_pointers[position]);
    if (next_node->node_type == LEAF) {
        auto* previous_node_leaf = new LeafNode(this->tree_pointers[position - 1]);
        auto* next_node_leaf = new LeafNode(this->tree_pointers[position]);

        // change leaf pointers
        previous_node_leaf->next_leaf_ptr = next_node_leaf->next_leaf_ptr;

        auto next_node_pointers = next_node_leaf->data_pointers;
        next_node_leaf->delete_node();

        for (auto it : next_node_pointers)
            previous_node_leaf->insert_key(it.first, it.second);

        this->keys[position - 1] = previous_node_leaf->max();
        if (position + 1 != this->size)
            this->keys.erase(this->keys.begin() + position);
        else
            this->keys.pop_back();
        this->tree_pointers.erase(this->tree_pointers.begin() + position);

        previous_node_leaf->dump();

        next_node_pointers.clear();
        delete previous_node_leaf;
        delete next_node_leaf;

    } else { // nodes are internal nodes
        auto* previous_node_internal = new InternalNode(this->tree_pointers[position - 1]);
        auto* next_node_internal = new InternalNode(this->tree_pointers[position]);

        auto all_pointers = previous_node_internal->tree_pointers;
        auto all_keys = previous_node_internal->keys;

        // insert max value of last node which is in the previous node
        auto* last_node = TreeNode::tree_node_factory(all_pointers.back());
        all_keys.push_back(last_node->max());
        all_keys.insert(all_keys.end(), next_node_internal->keys.begin(), next_node_internal->keys.end());
        all_pointers.insert(all_pointers.end(), next_node_internal->tree_pointers.begin(), next_node_internal->tree_pointers.end());

        previous_node_internal->size = all_pointers.size();
        previous_node_internal->keys = all_keys;
        previous_node_internal->tree_pointers = all_pointers;
        next_node_internal->delete_node();

        this->keys[position - 1] = previous_node_internal->max();
        if (position + 1 != this->size)
            this->keys.erase(this->keys.begin() + position);
        else
            this->keys.pop_back();
        this->tree_pointers.erase(this->tree_pointers.begin() + position);

        previous_node_internal->dump();

        all_pointers.clear();
        all_keys.clear();
        delete last_node;
        delete previous_node_internal;
        delete next_node_internal;
    }

    this->size -- ;
    delete next_node;
}

//deletes key from subtree rooted at this if exists
void InternalNode::delete_key(const Key &key) {
    for (uint i = 0 ; i < this->size ; i++ ) {
        if (i + 1 == this->size || key <= this->keys[i]) {
            auto* next_node = TreeNode::tree_node_factory(this->tree_pointers[i]);
            next_node->delete_key(key);

            // do nothing if no underflow occurs
            if (!next_node->underflows()) {
                if (i + 1 != this->size)
                    this->keys[i] = next_node->max();
                delete next_node;
                break;
            }

            // left sibling exists
            if (i > 0) {
                auto* left_node = TreeNode::tree_node_factory(this->tree_pointers[i - 1]);
                // redistribution can occur
                if (left_node->size + next_node->size >= 2 * MIN_OCCUPANCY)
                    redistribute_nodes(i);
                // merge can occur
                else
                    merge_nodes(i);

                delete left_node;
            } else { // right sibling exists
                auto* right_node = TreeNode::tree_node_factory(this->tree_pointers[i + 1]);
                // redistribution can occur
                if (right_node->size + next_node->size >= 2 * MIN_OCCUPANCY)
                    redistribute_nodes(i + 1);
                // merge can occur
                else
                    merge_nodes(i + 1);

                delete right_node;                
            }

            delete next_node;
            break;
        }
    }

    this->dump();
}

//runs range query on subtree rooted at this node
void InternalNode::range(ostream &os, const Key &min_key, const Key &max_key) const {
    BLOCK_ACCESSES++;
    for(int i = 0; i < this->size - 1; i++){
        if(min_key <= this->keys[i]){
            auto* child_node = TreeNode::tree_node_factory(this->tree_pointers[i]);
            child_node->range(os, min_key, max_key);
            delete child_node;
            return;
        }
    }
    auto* child_node = TreeNode::tree_node_factory(this->tree_pointers[this->size - 1]);
    child_node->range(os, min_key, max_key);
    delete child_node;
}

//exports node - used for grading
void InternalNode::export_node(ostream &os) {
    TreeNode::export_node(os);
    for(int i = 0; i < this->size - 1; i++)
        os << this->keys[i] << " ";
    os << endl;
    for(int i = 0; i < this->size; i++){
        auto child_node = TreeNode::tree_node_factory(this->tree_pointers[i]);
        child_node->export_node(os);
        delete child_node;
    }
}

//writes subtree rooted at this node as a mermaid chart
void InternalNode::chart(ostream &os) {
    string chart_node = this->tree_ptr + "[" + this->tree_ptr + BREAK;
    chart_node += "size: " + to_string(this->size) + BREAK;
    chart_node += "]";
    os << chart_node << endl;

    for(int i = 0; i < this->size; i++) {
        auto tree_node = TreeNode::tree_node_factory(this->tree_pointers[i]);
        tree_node->chart(os);
        delete tree_node;
        string link = this->tree_ptr + "-->|";

        if(i == 0)
            link += "x <= " + to_string(this->keys[i]);
        else if (i == this->size - 1) {
            link += to_string(this->keys[i-1]) + " < x";
        } else {
            link += to_string(this->keys[i-1]) + " < x <= " + to_string(this->keys[i]);
        }
        link += "|" + this->tree_pointers[i];
        os << link << endl;
    }
}

ostream& InternalNode::write(ostream &os) const {
    TreeNode::write(os);
    for(int i = 0; i < this->size - 1; i++){
        if(&os == &cout)
            os << "\nP" << i+1 << ": ";
        os << this->tree_pointers[i] << " ";
        if(&os == &cout)
            os << "\nK" << i+1 << ": ";
        os << this->keys[i] << " ";
    }
    if(&os == &cout)
        os << "\nP" << this->size << ": ";
    os << this->tree_pointers[this->size - 1];
    return os;
}

istream& InternalNode::read(istream& is){
    TreeNode::read(is);
    this->keys.assign(this->size - 1, DELETE_MARKER);
    this->tree_pointers.assign(this->size, NULL_PTR);
    for(int i = 0; i < this->size - 1; i++){
        if(&is == &cin)
            cout << "P" << i+1 << ": ";
        is >> this->tree_pointers[i];
        if(&is == &cin)
            cout << "K" << i+1 << ": ";
        is >> this->keys[i];
    }
    if(&is == &cin)
        cout << "P" << this->size;
    is >> this->tree_pointers[this->size - 1];
    return is;
}
