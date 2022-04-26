#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <mutex>
#include<unordered_set>
#include <time.h>
#include <string>

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
    mutex *lock; //for rake when 2 children try to rake up to same parent
    bool coinflip; //for compress to decide which nodes to comprress
};
typedef struct node* Tree_Node;


class ExpressionTreeSolve {
    public:
        Tree_Node Tree;
        std::unordered_set<Tree_Node> Nodes;
        std::unordered_set<Tree_Node> Leaves;

        //INITIALIZE AND INITIALIZE METDATA HELPERS
        void init() {
            initialize_nodes(Tree);
            initialize_leaves(Tree);
        };

        void initialize_nodes(Tree_Node T) {
            if (T == NULL) {
                return;
            }
            else {
                Nodes.insert(T);
                initialize_nodes(T->left);
                initialize_nodes(T->right);
            }
        }

        void initialize_leaves(Tree_Node T) {
            if (T == NULL) {
                return;
            } 
            if (T->left == NULL && T->right == NULL) {
                Leaves.insert(T);
            }
            else {
                initialize_leaves(T->left);
                initialize_leaves(T->right);
            }
        }

        // FREE HELPER
        void free_node(Tree_Node node){
            free(node->data);
            free(node->lock);
            free(node);
        }

        //RAKE AND RAKE HELPERS
        void* rake_thread(void* leaf){
            //get args
            Nodes.erase((Tree_Node)leaf);
            
            Tree_Node parent = ((Tree_Node)leaf)->parent;
            mutex *parent_lock = parent->lock;
            int leaf_num = ((Tree_Node)leaf)->data->num;
            
            //lock parent_lock (another child might get there first)
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
        }

        void rake(){ //to rake is all the leaves

            // Create local copy of current leaves and empty global leaves for newly formed leaves to be inserted 
            std::unordered_set<Tree_Node> Leaves_local = Leaves;
            Leaves.clear();

            //spawn threads to do all leaves in parallel
            pthread_t* threads = (pthread_t*)calloc(Leaves_local.size(), sizeof(pthread_t));

            //iterate through leaves
            for (unordered_set<Tree_Node>::iterator leaf = Leaves_local.begin(); leaf != Leaves_local.end(); ++leaf) {
                //spawn thread
                pthread_t thread_id;
                *threads = thread_id;
                threads = threads + sizeof(pthread_t); //keep track of threads to join later
                pthread_create(&thread_id, NULL, this->rake_thread, (void*)(&(*leaf)));
            }
            //join threads (waits)
            for (int i=0; i<Leaves_local.size(); i++){
                pthread_join(threads[i], NULL);
            }
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
            bool only_child = parent->right == NULL || parent->left == NULL;
            // all above true for compress
            return me_head && parent_tail && left_tail && right_tail && one_child; 
        }

        void *compress_thread(void *tree_node) {
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

        void compress(){
            //spawn threads to do all leaves in parallel
            thread* threads = (thread*)calloc(Nodes.size(), sizeof(thread));

            //iterate through nodes
            int counter = 0;
            for (unordered_set<Tree_Node>::iterator tree_node = Nodes.begin(); tree_node != Nodes.end(); ++tree_node) {
                //spawn thread
                // thread thread_id;
                threads[counter] = thread([this] { this->assign_coinflips(tree_node); });

                counter += 1;
                // pthread_create(&thread_id, NULL, (void*(*)(void*)) &ExpressionTreeSolve::assign_coinflips, (void*)(&(*tree_node)));
            }

            //join threads
            for (int i=0; i<Nodes.size(); i++){
                threads[i].join();
            }

            //spawn threads to do all leaves in parallel
            pthread_t* threads = (pthread_t*)calloc(Nodes.size(), sizeof(pthread_t));

            //iterate through nodes
            for (unordered_set<Tree_Node>::iterator tree_node = Nodes.begin(); tree_node != Nodes.end(); ++tree_node) {
                //spawn thread
                pthread_t thread_id;
                *threads = thread_id;
                threads = threads + sizeof(pthread_t);
                pthread_create(&thread_id, NULL, (void*(*)(void*)) &ExpressionTreeSolve::compress_thread, (void*)(&(*tree_node)));
            }

            //join threads
            for (int i=0; i<Nodes.size(); i++){
                pthread_join(threads[i], NULL);
            }

        }

        //SOLVE
        int solve(){ 
            while (Nodes.size() > 1) {
                    rake();
                    compress();
            }
            return Tree->data->num;
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
        Tree_Node make_tree_from_list(string *chars, int len) { //sadly i don't think this is possible/easy
            //make a list to keep track of nodes indices
            Tree_Node* list_of_nodes = (Tree_Node *)calloc(len, sizeof(Tree_Node));
            for(int i = 0; i < len; i++) {
                Tree_Node node = make_node_from_char(chars[i]);
                Tree_Node parent = NULL;
                if (i != 0) {
                    parent = list_of_nodes[i/2];
                    node->parent = parent;
                }
                if (i % 2) { // right child
                    parent->right = node;
                }
                else { // left child
                    parent->left = node;
                }
            }
            Tree = list_of_nodes[0];
        }
        
        Tree_Node make_node(struct node_data* data){
            mutex lock;
            Tree_Node tree_node = (Tree_Node)malloc(sizeof(struct node));
            tree_node->data = data;
            tree_node->lock = &lock;
            return tree_node; //does not initialize parent
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

int main(){
    //test cases
    ExpressionTreeSolve Solver = ExpressionTreeSolve();
    // 1
    string tree[1] = {"1"};
    Solver.make_tree_from_list(tree, 1);
    Solver.init();
    assert(Solver.solve() == 1);
    // 2
    string tree[3] = {"+", "5", "6"};
    Solver.make_tree_from_list(tree, 3);
    Solver.init();
    assert(Solver.solve() == 11);
}