#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include<unordered_set>
#include <time.h>

// TODO: check if can do parametrized classes



typedef int (*inner_lambda) (int);
typedef inner_lambda(*outer_lambda)(int);

union lambda
{
    inner_lambda inner;
    outer_lambda outer;
};

struct lambda_data
{
    union lambda fn;
    bool outer_flag;
}

union node_data //all leaves are nums, else: lambda
{
    int num;
    struct lambda_data fn_data;
};

struct node //Tree representation 
{
    union node_data* data;
    
    struct node* left;
    struct node* right;
    struct node* parent;
    pthread_mutex_t *lock; 
};
typedef struct node* Tree_Node;

struct rake_thread_args
{
    Tree_Node leaf;
    Tree_Node parent;
    // pthread_mutex_t* parent_lock;
};

class ExpressionTreeSolve {
    public:
        Tree_Node Tree;
        // int num_nodes;
        // Tree_Node *Tree_Array;
        // pthread_mutex_t *Locks;
        std::unordered_set<Tree_Node> Nodes = {};
        std::unordered_set<Tree_Node> Leaves = {};

        void init(Tree_Node T) {
            Tree = T;
            create_tree_array_help(Tree, Tree_Array, 0);
            // ^ or initialize_tree_array() ---> prettiness vs looking like there's more code
            initialize_lock_array();
            initialize_leaves(T);
        };

        int solve(Tree_Node root, int l){ 
            rake();
            compress();
            // rake
            // update leaves
            // compress



            //n = len(tree)
            //l = len(leaves) (to rake)
            //c = len(to_compress)
            

            //need to somehow do parallelism here?
            //i = me, 2i= left child, 2i+1 = right child
            //for all even nodes j, check if j+1 is empty or not. if empty: compress, else: rake
        }

        // int create_tree_array_help(Tree_Node T, Tree_Node *A, int id) {
        //     if (T == NULL) {
        //         return id;
        //     }
        //     int id_left = create_tree_array_help(T->left, A, id);
        //     A[0 + id_left] = T;
        //     T->id = id_left;
        //     int id_right = create_tree_array_help(T->right, A, id_left + 1);
        //     return id_right;
        // };
        // //create tree array from tree and fill index id in nodes (map node index to node address). If have time do the parallel in-order traversal from lecture. 
        // void initialize_tree_array() {
        //     Tree_Node *A = (Tree_Node*)(calloc((size_t)num_nodes, sizeof(Tree_Node)));
        //     create_tree_array_help(Tree, A, 0);
        //     Tree_Array = A;
        // }

        void initialize_nodes_set(Tree_Node T) {
            if (T == NULL) {
                return;
            }
            else {
                Nodes.insert(T);
                initialize_nodes_set(T->left);
                initialize_nodes_set(T->right);
            }
        }

        // //create lock list: index -> lock (1 is locked, 0 is available)
        // void initialize_lock_array() {
        //     pthread_mutex_t *Locks = (pthread_mutex_t*)calloc((size_t) num_nodes, sizeof(pthread_mutex_t));
        // }
        
        // initialize set of leaves
        void initialize_leaves(Tree_Node T) {
            if (T == NULL) {
                return;
            } 
            if (T->left == NULL && T->right == NULL) {
                Leaves.insert(T->id);
            }
            else {
                initialize_leaves(T->left);
                initialize_leaves(T->right);
            }
        }

        //TODO: free old node data
        void* rake_thread(void* args){

            //get args
            Tree_Node parent = ((rake_thread_args*)args)->parent;
            Tree_Node leaf = ((rake_thread_args*)args)->leaf;
            pthread_mutex_t* parent_lock = ((rake_thread_args*)args)->parent_lock;

            //lock parent_lock (another child might get there first)
            pthread_mutex_lock(parent_lock); 
            
            if (parent->data->lambda_data->outer_flag){ //outer case

                outer_lambda outer = parent->data->fn_data->fn.outer;
                inner_lambda inner = outer(leaf->data->num); //all leaves are nums

                parent->data->fn_data->outer_flag = false;
                parent->data->fn_data->fn.inner = inner
            }
            else{ //inner case

                inner_lambda inner = parent->data->fn_data->fn.inner;
                parent->data->num = inner(leaf->data->num); //all leaves are nums

                Leaves.insert(parent->id); //new leaves is parent
                //shouldn't we delete old leaf too?
            }
            
            //unlock parent_lock
            pthread_mutex_unlock(parent_lock); 
        }

        void rake(){ //to rake is all the leaves

            // Create local copy of current leaves and empty global leaves for newly formed leaves to be inserted 
            std::unordered_set<int> Leaves_local = Leaves;
            Leaves.clear();

            //spawn threads to do all leaves in parallel
            pthread_t* threads = (pthread_t*)calloc(Leaves_local.size(), sizeof(pthread_t));

            //iterate through leaves
            for (int it = Leaves_local.begin(); it != Leaves_local.end(); ++it) {
                
                Tree_Node leaf = Tree_Array[*it];
                Tree_Node parent = leaf->parent;
                pthread_mutex_t* parent_lock = &Locks[parent->id];

                //args to pass into thread
                rake_thread_args args;
                args = (rake_thread_args){
                    .leaf = leaf,
                    .parent = parent,
                    .parent_lock = parent_lock
                };

                //spawn thread
                pthread_t thread_id;
                *threads = thread_id;
                threads = threads + sizeof(pthread_t);
                pthread_create(&thread_id, NULL, (void*(*)(void*)) &ExpressionTreeSolve::rake_thread, (void*)&args);
            }

            //join threads
            for (int i=0; i<Leaves_local.size(); i++){
                pthread_join(threads[i], NULL);
            }

        }

        void compress(){
            // assign random numbers
            //lee fucking mao, do something here
                        // Create local copy of current leaves and empty global leaves for newly formed leaves to be inserted 

            //spawn threads to do all leaves in parallel
            pthread_t* threads = (pthread_t*)calloc(num_nodes, sizeof(pthread_t));

            //iterate through leaves
            for (auto it = Leaves_local.begin(); it != Leaves_local.end(); ++it) {
                Tree_Node leaf = Tree_Array[*it];
                Tree_Node parent = leaf->parent;
                pthread_mutex_t* parent_lock = &Locks[parent->id];

                //args to pass into thread
                rake_thread_args args;
                args = (rake_thread_args){
                    .leaf = leaf,
                    .parent = parent,
                    .parent_lock = parent_lock
                };

                //spawn thread
                pthread_t thread_id;
                *threads = thread_id;
                threads = threads + sizeof(pthread_t);
                pthread_create(&thread_id, NULL, (void*(*)(void*)) &ExpressionTreeSolve::rake_thread, (void*)&args);
            }

            //join threads
            for (int i=0; i<Leaves_local.size(); i++){
                pthread_join(threads[i], NULL);
            }
        }

};



