#ifndef STRUCTURE
#define STRUCTURE


// class structure
// {
//     private:
//         // int count;
//         double tomato;
//         double onion;
//         double pepper;
    
//     public:
//         int count;
//         // int getcount();
//     //     structure(int count, double tomato, double onion, double pepper);
//     //     void display();
        
// };

struct structure{
    int count;
    int num_of_salads_produced;
    bool done;
    int tomato;
    int onion;
    int pepper;
    int num_concurrent_processes;
    double salmaker1_preparetime;
    double salmaker2_preparetime;
    double salmaker3_preparetime;
    double salmaker1_waittime;
    double salmaker2_waittime;
    double salmaker3_waittime;

};

// struct salad_maker_ingredients{ //sth is fishy with this its giving seg faults
//     int onion;
//     int tomato;
//     int pepper;
// };



#endif