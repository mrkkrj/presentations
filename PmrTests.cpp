#include <iostream>

#include <vector>
#include <array>
#include <list>
#include <memory_resource>


// graph 

struct GraphNode {
   std::pmr::string m_payload;
   std::pmr::vector<GraphNode*> m_outgoingEdges;
   explicit GraphNode(const std::pmr::string& payload, std::pmr::memory_resource* alloc);
   ~GraphNode() { }
};

GraphNode::GraphNode(const std::pmr::string& payload, std::pmr::memory_resource* alloc)
   : m_payload(payload, alloc), m_outgoingEdges(alloc) {
     m_outgoingEdges.reserve(2); // Typical fan-out is 2.
}

// allocator aware graph
struct AAGraphNode {
   using allocator_type = std::pmr::polymorphic_allocator<char>;

   std::pmr::string m_payload;
   std::pmr::vector<GraphNode*> m_outgoingEdges;
   explicit AAGraphNode(const std::pmr::string& payload, const allocator_type&  alloc);
   ~AAGraphNode() { }
};

AAGraphNode::AAGraphNode(const std::pmr::string& payload, const allocator_type& alloc)
   : m_payload(payload, alloc), m_outgoingEdges(alloc) {
     m_outgoingEdges.reserve(2); // Typical fan-out is 2.
}

// -- tests --

int main()
{ 
    // 1.
    std::array<unsigned, 44> arr{};
    std::pmr::monotonic_buffer_resource arary_res_1(arr.data(), arr.size() * sizeof(unsigned));

    std::pmr::vector<int> vec1(&arary_res_1);
    vec1.push_back(-1);
    vec1.push_back(0);
    vec1.push_back(1);

    // 2.
    unsigned buff[1024] = {};
    std::pmr::monotonic_buffer_resource arary_res_2(buff, sizeof(buff));

    std::pmr::vector<std::string> vec2(&arary_res_2);
    vec2.push_back("alpha");
    vec2.push_back("beta");
    vec2.push_back("gamma");

    // 3.
    std::pmr::vector<unsigned> vec3({1, 2, 3, 4}, &arary_res_2);

#if 1 // my virus scaner runs amok :-(
    // 4. 
    std::pmr::vector<std::pmr::string> vec4(&arary_res_2);
    vec4.push_back("A");
    vec4.push_back("B");
    vec4.push_back("CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC"); // to beat SSO!
#endif

    // 5. using nonotonic resource in a loop
    unsigned buffer[1024] = {};
    std::pmr::monotonic_buffer_resource buffer_mem_res(buffer, sizeof(buffer));
    const int N = 10000;

    for (int i = 0; i < N; ++i)
    {
       buffer_mem_res.release(); // rewind to the beginning

       std::pmr::vector<std::string> strg_vec(&buffer_mem_res);
       strg_vec.push_back("strg XXX");
    }


    // wink-out
    std::pmr::unsynchronized_pool_resource pool_resrc;

    {

#if __cplusplus >= 202002L

      // C++ 20

      std::pmr::vector<std::pmr::list<std::pmr::string>>& data =
         //*new(&pool_resrc) std::pmr::vector<std::pmr::list<std::pmr::string>>(&pool_resrc); --> crash!
         *pool_alloc.new_object<std::pmr::vector<std::pmr::list<std::pmr::string>>>();
#else
     
       // crash!!!      
    #if 0        
       std::pmr::vector<std::pmr::list<std::pmr::string>>& data = 
           *new(&pool_resrc) std::pmr::vector<std::pmr::list<std::pmr::string>>(&pool_resrc);
    #else
      std::pmr::polymorphic_allocator<unsigned> pool_alloc(&pool_resrc); 
      using VecListStrg = std::pmr::vector<std::pmr::list<std::pmr::string>>;

      void* p = pool_alloc.allocate(sizeof(VecListStrg));
      auto vec = static_cast<VecListStrg*>(p);      

      pool_alloc.construct<VecListStrg>(vec /*, &pool_resrc*/); // PITA! - 2969. polymorphic_allocator::construct() shouldn't pass resource() 

      // OR:
      void* p1 = pool_alloc.allocate(sizeof(VecListStrg));
      VecListStrg* vec1 = new(p1) VecListStrg(&pool_resrc); // PITA?

      // on the stack:
      VecListStrg vec2(&pool_resrc);

      auto& data = *vec;
    #endif
#endif    

       // ... Build up and use 'data' here ...
       data.push_back({});
       data.push_back({});

       data[0].push_back("string XXX");
       data[0].push_back("string YYY");             
       data[1].push_back("string ZZZ");
    }

    // out of scope, data gets winked out!

    // no need to call 'pool_resrc.deleteObject(&data)' ????


    // graph usage 
    //  - using self contained heap (aka localized GC)

    {
#if __cplusplus >= 202002L

      // C++ 20

      GraphNode* start = //new(&mem_resrc) GraphNode("start", &mem_resrc);
         buff_alloc.new_object<GraphNode>("start", &mem_resrc);  // C++20

#else

       unsigned buffer[2048] = {};
       std::pmr::monotonic_buffer_resource mem_resrc(buffer, sizeof(buffer));

       // crashing!!!      
    #if 0
       GraphNode* start = new(&mem_resrc) GraphNode("start", &mem_resrc);
       // ...
       GraphNode* nX = new(&mem_resrc) GraphNode("nodeX", &mem_resrc);
    #else
      std::pmr::polymorphic_allocator<unsigned> buff_alloc(&mem_resrc); 

      GraphNode* start = static_cast<GraphNode*>((void*)buff_alloc.allocate(sizeof(GraphNode)));
      buff_alloc.construct<GraphNode>(start, "start", &mem_resrc);
            
      GraphNode* nX = static_cast<GraphNode*>((void*)buff_alloc.allocate(sizeof(GraphNode)));
      buff_alloc.construct<GraphNode>(nX, "nodeX", &mem_resrc);
    #endif

#endif
       
       // cycles are no problem!!!
       start->m_outgoingEdges.push_back(nX);
       nX->m_outgoingEdges.push_back(start);

       // 'mem_resrc' destructor releases memory!
    }

    // AA-Graph

    {               
       char buffer[2048] = {};
       std::pmr::monotonic_buffer_resource mem_resrc(buffer, sizeof(buffer));

      std::pmr::polymorphic_allocator<char> buff_alloc(&mem_resrc); 

      AAGraphNode* start = static_cast<AAGraphNode*>((void*)buff_alloc.allocate(sizeof(AAGraphNode)));
      buff_alloc.construct<AAGraphNode>(start, "start"); // ??????
            
      AAGraphNode* nX = static_cast<AAGraphNode*>((void*)buff_alloc.allocate(sizeof(AAGraphNode)));
      buff_alloc.construct<AAGraphNode>(nX, "nodeX");
    }


    // ready
    std::cout << "finished!\n";

#if __cplusplus >= 202002L
      std::cout << " - C++20\n";
#else
      std::cout << " - C++17\n";
#endif

}
