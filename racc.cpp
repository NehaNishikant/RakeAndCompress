#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <future>
#include <mutex>
#include<unordered_set>
#include <time.h>
#include <string>
#include <vector>
#include <iostream>
#include <memory>

using namespace std;


struct node_data //all leaves are nums, else: op
{
    //leaves (undefined if non-leaf):
    int num;

    //non leaves (undefined if leaf):
    bool op; //true= +, false= *
    int alpha; //todo: initialize to 1
    int beta; //todo: initialize to 0
};

struct node //Tree representation 
{
    struct node_data* data;
    
    struct node* left;
    struct node* right;
    struct node* parent;

    //for rake when 2 children try to rake up to same parent
    // int mutex_id;
    mutex* lock;

    //for compress to decide which nodes to comprress
    bool coinflip; 
};
typedef struct node* Tree_Node;


class ExpressionTreeSolve {
    public:
        Tree_Node Tree;
        unordered_set<Tree_Node> Nodes;
        unordered_set<Tree_Node> Leaves;
        //vector<mutex> mutexes;

        //INITIALIZE AND INITIALIZE METDATA HELPERS
        void init() {
            initialize_nodes(Tree);
        };

        void initialize_nodes(Tree_Node T) {
            if (T == NULL) {
                return;
            }
            else {
                Nodes.insert(T);
                //mutex new_m;
                //mutexes.push_back(new_m);

                if (T->left == NULL && T->right == NULL){
                    Leaves.insert(T);
                }
                initialize_nodes(T->left);
                initialize_nodes(T->right);
            }
        }

        // FREE HELPER
        void free_node(Tree_Node node){
            free(node->data);
            // free(node->lock);
            free(node);
        }

        //RAKE AND RAKE HELPERS
        void rake_thread(promise<Tree_Node> && p, Tree_Node leaf){
            Nodes.erase((Tree_Node)leaf);

            //get args
            Tree_Node parent = ((Tree_Node)leaf)->parent;
            //parent_lock = mutexes[parent->mutex_id];
            int leaf_num = ((Tree_Node)leaf)->data->num;
            
            //lock parent_lock (another child might get there first)
            mutex* parent_lock = parent->lock;
            parent_lock->lock();
            
            if (parent->left != NULL & parent->right != NULL){ //first leaf (update lambda)
                if (parent->data->op){
                    //plus
                    parent->data->beta += leaf_num;
                } else{
                    //multiplication
                    parent->data->alpha *= leaf_num;
                    parent->data->beta *= leaf_num;
                }
            } else { //second leaf (resolve lambda)
                parent->data->num += parent->data->alpha * leaf_num + parent->data->beta;
            }
            
            //clean up leaf
            if (parent->right == leaf){
                parent->right = NULL;
            } else{
                parent->left = NULL;
            }

            free_node((Tree_Node)leaf);
            

            //unlock parent_lock
            parent_lock->unlock();

            //return parent
            p.set_value(parent);
        }

        void rake(){ //to rake is all the leaves

            //spawn threads to do all leaves in parallel
            std::vector<std::thread> threads;
            vector< std::future<Tree_Node> > futures; //to collect parents of raked leaves
            
            //iterate through leaves
            for (unordered_set<Tree_Node>::iterator leaf = Leaves.begin(); leaf != Leaves.end(); ++leaf) {
                //spawn thread
                std::promise<Tree_Node> p;
                futures.push_back(p.get_future());
                threads.push_back(thread(&ExpressionTreeSolve::rake_thread, this, move(p), (Tree_Node)*leaf));
            }
            //wait for threads to finish
            for (auto& th : threads) th.join();

            //add parent of raked leaves to Leaves
            Leaves.clear();
            for (auto& f : futures) Leaves.insert(f.get());
        }

        //COMPRESS + COMPRESS HELPERS
        void assign_coinflips(Tree_Node tree_node) {
            ((Tree_Node)tree_node)->coinflip = (bool)(rand() % 2);
        }

        bool should_compress(Tree_Node tree_node) {
            // im a head
            bool me_head = tree_node->coinflip;
            // not root and parent tail
            Tree_Node parent = tree_node->parent;
            bool parent_tail = parent != NULL && !parent->coinflip;
            // left child tail (if it exists)
            bool left_tail = tree_node->left == NULL || !tree_node->left->coinflip;
            // right child tail (if it exists)
            bool right_tail = tree_node->right == NULL || !tree_node->right->coinflip;
            // i have exactly one child (being a leaf complicates things)
            bool one_child = (tree_node->right == NULL || tree_node->left == NULL) && (tree_node->right != NULL || tree_node->left != NULL); 
            // im an only child
            bool only_child = parent != NULL && (parent->right == NULL || parent->left == NULL);
            // all above true for compress
            return me_head && parent_tail && left_tail && right_tail && one_child; 
        }

        void compress_thread(Tree_Node tree_node) {

            // based on coinflips, only compress selected nodes
            if (!should_compress((Tree_Node)tree_node)){ 
                return;
            }
            
            // combine alpha beta of node with parent node's alpha beta
            Tree_Node node = (Tree_Node)tree_node;
            Tree_Node parent = ((Tree_Node)tree_node)->parent;
            parent->data->alpha *= node->data->alpha;
            parent->data->beta *= parent->data->alpha * node->data->beta + parent->data->beta;
            
            // restructure tree 
            parent->left = ((Tree_Node)tree_node)->left;
            parent->right = ((Tree_Node)tree_node)->right;
            
            // remove and free node. TODO: garbage collection
            Nodes.erase((Tree_Node)tree_node);
            free_node((Tree_Node)tree_node);
        }

        void compress(){ //TODO: free threads everywhere
        
            
            std::vector<std::thread> coinflip_threads;

            //iterate through nodes
            for (unordered_set<Tree_Node>::iterator tree_node = Nodes.begin(); tree_node != Nodes.end(); ++tree_node) {
                //spawn thread
                coinflip_threads.push_back(thread(&ExpressionTreeSolve::assign_coinflips, this, (Tree_Node)*tree_node));
            }

            

            //wait for threads to finish
            for (auto& th : coinflip_threads) th.join();

            //spawn threads to do all leaves in parallel
            std::vector<std::thread> compress_threads;

            //iterate through nodes
            for (unordered_set<Tree_Node>::iterator tree_node = Nodes.begin(); tree_node != Nodes.end(); ++tree_node) {
                //spawn thread
                compress_threads.push_back(thread(&ExpressionTreeSolve::compress_thread, this, (Tree_Node)*tree_node));
            }

            //wait for threads to finish
            for (auto& th : compress_threads) th.join();

        }

        //SOLVE
        int solve(){ 
            while (Nodes.size() > 1) {
                rake();
                // return 11;
                compress();
            }

            Nodes.clear();
            Leaves.clear();
            int x = Tree->data->num;
            //TODO: free tree
            return x;
        }

        Tree_Node make_node_from_char(string a) {
            if (a == "NULL") {
                return NULL;
            }
            if (a == "+") {
                return make_op_node(true);
            }
            if (a == "*"){
                return make_op_node(false);
            }
            return make_num_node(stoi(a));
        }

        //INPUT TREE INITIALIZATION HELPERS
        void make_tree_from_list(vector<string>& chars, int len) {
            //make a list to keep track of nodes indices
            Tree_Node* list_of_nodes = (Tree_Node *)calloc(len, sizeof(Tree_Node));
            for(int j = 0; j < chars.size(); j++){
                Tree_Node node = make_node_from_char(chars[j]);

                int i = j+1;
                Tree_Node parent = NULL;
                if (i != 1) {
                    parent = list_of_nodes[i/2 - 1];
                }

                if (node != NULL){
                    node->parent = parent;
                    node->left = NULL;
                    node->right = NULL;
                }
                
                if (parent != NULL){
                    if (i % 2) { // right child
                        parent->right = node;
                    }
                    else { // left child
                        parent->left = node;
                    }
                }
                list_of_nodes[j] = node;
            }
            Tree = list_of_nodes[0];
        }
        
        Tree_Node make_node(struct node_data* data){
            //mutex lock;
            Tree_Node tree_node = (Tree_Node)malloc(sizeof(struct node));
            tree_node->data = data;
            tree_node->lock = new mutex();
            return tree_node; //does not initialize parent and child pointers
        }

        Tree_Node make_op_node(bool plus){
            struct node_data* data = (struct node_data*)(malloc(sizeof(struct node_data)));
            data->op = plus;
            data->alpha = 1;
            data->beta = 0;
            return make_node(data);
        }

        Tree_Node make_num_node(int num){
            struct node_data* data = (struct node_data*)malloc(sizeof(struct node_data));
            data->num = num;
            return make_node(data);
        }

};

int main(int argc, char** argv){
    //test cases
    ExpressionTreeSolve Solver = ExpressionTreeSolve();
    // 1
    vector<string> tree1;
    tree1.push_back("1");
    Solver.make_tree_from_list(tree1, 1);

    Solver.init();
    assert(Solver.solve() == 1);
    cout << "Test case 1 passed\n";

    //2
    vector<string> tree2;
    tree2.push_back("+");
    tree2.push_back("5");
    tree2.push_back("6");
    Solver.make_tree_from_list(tree2, 3);
    Solver.init();
    assert(Solver.solve() == 11);
    cout << "Test case 2 passed\n";
}