//
// C++20 code examples for PMR usage
//  - using trace and debug outputs
//

#include <memory_resource>
#include <list>
#include <string>
#include <vector>
#include <array>
#include <iostream>


// test trace
#define LOG_TESTCASE(a) std::cout << "\n\n >>>>>>> TEST:: " << a << " - \n\n"


// 
// A simple tracing debug memory resource
//  - code based on Sticky Bits and Pablo Halpern's talk.
//
class debug_resource
	: public std::pmr::memory_resource
{
public:
	explicit debug_resource(std::string name, 
							std::pmr::memory_resource* up = std::pmr::get_default_resource()) // normally the ::new() operator!
	  : _name{ std::move(name) }, 
		_upstream{ up }
	{ }

	void* do_allocate(size_t bytes, size_t alignment) override 
	{
		std::cout << " -- '" << _name << "' do_allocate(): " << bytes << '\n';
		void* ret = _upstream->allocate(bytes, alignment);
		return ret;
	}

	void do_deallocate(void* ptr, size_t bytes, size_t alignment) override 
	{
		std::cout << " -- '" << _name << "' do_deallocate(): " << bytes << '\n';
		_upstream->deallocate(ptr, bytes, alignment);
	}

	bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override 
	{
		return this == &other;
	}

private:
	std::string _name;
	std::pmr::memory_resource* _upstream;
};


//
// helper function
//  - debug prints the underlying buffer of a PMR resource
//
void printBufferContents(std::string_view buf, std::string_view title)
{
	std::cout << " -- " << title << ":\n";

	for (size_t i = 0; i < buf.size(); ++i) 
	{
		std::cout << (buf[i] >= ' ' ? buf[i] : (buf[i] == 0 ? '.' : '-'));
		if ((i + 1) % 64 == 0) std::cout << '\n';
	}

	std::cout << '\n';
}


//
//  Some PMR-aware class
//
struct SomeProduct_Pmr 
{
	using allocator_type = std::pmr::polymorphic_allocator<char>;

	explicit SomeProduct_Pmr(allocator_type alloc = {}) 
	  : _name{ alloc } 
	{ }

	SomeProduct_Pmr(std::pmr::string name, char price, const allocator_type& alloc = {}) 
	  : _name{ std::move(name), alloc }, 
		_price{ price } 
	{ }

	SomeProduct_Pmr(const SomeProduct_Pmr& other, const allocator_type& alloc) 
	  : _name{ other._name, alloc }, 
		_price{ other._price } 
	{ }

	SomeProduct_Pmr(SomeProduct_Pmr&& other, const allocator_type& alloc) 
	  : _name{ std::move(other._name), alloc }, 
		_price{ other._price } 
	{ }

	SomeProduct_Pmr& operator=(const SomeProduct_Pmr& other) = default;
	SomeProduct_Pmr& operator=(SomeProduct_Pmr&& other) = default;

	std::pmr::string _name;
	char _price{' '};
};


//
//  The usual PMR-unaware class
//
struct SomeProduct 
{
	std::pmr::string _name;
	char _price{ ' ' };
};


//
// Graph class for the Localized-GC technique demo
//
struct GraphNode
{
	std::pmr::string m_payload;
	std::pmr::vector<GraphNode*> m_outgoingEdges;

	explicit GraphNode(const std::pmr::string& payload, std::pmr::memory_resource* alloc);
	~GraphNode() { }
};

GraphNode::GraphNode(const std::pmr::string& payload, std::pmr::memory_resource* alloc)
	: m_payload(payload, alloc),
	m_outgoingEdges(alloc)
{
	m_outgoingEdges.reserve(2); // typical fan-out is 2.
}


// -- main ---

int main()
{
	{
		LOG_TESTCASE("monotonic_buffer_resource - std::array<int>");

		// 1. ints
		std::array<int, 44> arr{};
		std::fill_n((char*)&arr[0], 44, '_');

		std::pmr::monotonic_buffer_resource arary_res_1(arr.data(), arr.size() * sizeof(unsigned));

		std::pmr::vector<int> vec1(&arary_res_1);
		vec1.push_back(-1);
		vec1.push_back(0);
		vec1.push_back(1);

		printBufferContents(std::string_view{ (char*)&arr[0], 44 }, " 1. after 3 insertions --> VEC_1 - integers");
	}

	{
		LOG_TESTCASE("monotonic_buffer_resource - std::byte[]");

		// 2. std::strings
		std::byte buff[1024] = {};
		std::fill_n((char*)buff, std::size(buff), '_');

		std::pmr::monotonic_buffer_resource arary_res_2(buff, sizeof(buff));

		// --> get the SSO buffer size :D
		auto ssoSize = std::string{}.capacity();
		std::cout << " --- std::string's SSO buffer size:" << ssoSize << "\n" << std::endl;

		std::pmr::vector<std::string> vec2(&arary_res_2);
		vec2.push_back("alpha"); // SSO
		vec2.push_back("beta");
		vec2.push_back("gamma");
		vec2.push_back("CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC"); // to beat SSO!

		printBufferContents(std::string_view{ (char*)buff, 1024 }, " 2. after 4 insertions --> VEC_2 - std::strings");

		// 3. unsigned
		std::pmr::vector<unsigned> vec3({ 1, 2, 3, 4 }, &arary_res_2);

		printBufferContents(std::string_view{ (char*)buff, 1024 }, " 3. after 1 insertion --> VEC_3 - integers");

		// 4. pmr::strings
		std::pmr::vector<std::pmr::string> vec4(&arary_res_2);
		vec4.push_back("eta");
		vec4.push_back("theta");
		vec4.push_back("zeta");
		vec4.push_back("CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC"); // to beat SSO!

		printBufferContents(std::string_view{ (char*)buff, 1024 }, " 4. after 4 insertions --> VEC_4 - pmr::strings");
	}

	// 5. usage of monotonic_buffer_resource in a loop
	{
		LOG_TESTCASE("monotonic_buffer_resource in a loop");

		unsigned buffer[1024] = {};
		std::pmr::monotonic_buffer_resource buffer_mem_res(buffer, sizeof(buffer));
		const int N = 10000;

		for (int i = 0; i < N; ++i)
		{
			buffer_mem_res.release(); // rewind to the beginning

			std::pmr::vector<std::string> strg_vec(&buffer_mem_res);
			strg_vec.push_back("strg XXX");
		}

		printBufferContents(std::string_view{ (char*)buffer, 1024 }, " 5. after 10000 insertions --> std::strings");
	}

	// 6. wink-out
	{
		LOG_TESTCASE("wink-out");

		std::pmr::unsynchronized_pool_resource pool_resrc;
		std::pmr::polymorphic_allocator<std::byte> pool_alloc(&pool_resrc);

		{
			std::pmr::vector<std::pmr::list<std::pmr::string>>* dataPtr =
				//new(&pool_resrc) std::pmr::vector<std::pmr::list<std::pmr::string>>(&pool_resrc); // not supported!				
				pool_alloc.new_object<std::pmr::vector<std::pmr::list<std::pmr::string>>>();			// C++20: !!!
			
			// Build up and use 'data' here ...
			auto& data = *dataPtr;

			data.push_back({});
			data.push_back({});

			data[0].push_back("string XXX XXX");
			data[0].push_back("string YYY YYY");
			data[1].push_back("string ZZZ ZZZ");
			data[1].push_back("short"); // SSO?

			// OPEN TODO::: global allocator used (as PMR not forwarded)?
			//  -- not in our case, but it depends on the SSO-size!
			auto ssoSize = std::pmr::string{}.capacity();
			std::cout << " --- std::string's SSO buffer size:" << ssoSize << "\n" << std::endl;
		}

		// out of scope, data gets winked out!
		//  -- no need to call 'pool_resrc.deleteObject(dataPtr)'
	}

	// 7. graph example using self contained heap (aka localized GC)
	{
		LOG_TESTCASE("localized GC");

		unsigned buffer[2048] = {};
		std::pmr::monotonic_buffer_resource mem_resrc(buffer, sizeof(buffer));
		std::pmr::polymorphic_allocator<> buff_alloc(&mem_resrc); // C++ 20

		GraphNode* start = //new(&mem_resrc) GraphNode("start", &mem_resrc);		  // not supported!
								 buff_alloc.new_object<GraphNode>("start", &mem_resrc); // C++20

		// ... more nodes...

		GraphNode* nX = //new(&mem_resrc) GraphNode("nodeX", &mem_resrc);		  // not supported!
							 buff_alloc.new_object<GraphNode>("nodeX", &mem_resrc); // C++ 20

		// cycles are no problem!!!
		start->m_outgoingEdges.push_back(nX);
		nX->m_outgoingEdges.push_back(start);

		// 'mem_resrc' destructor releases memory!
	}

	// 8. NEW:::
	//  --> using debug resource
	{
		LOG_TESTCASE("debug resource - std::string");

		constexpr size_t BUF_SIZE = 256;
		char bufferSm[BUF_SIZE] = {}; // a small buffer on the stack
		std::fill_n(std::begin(bufferSm), std::size(bufferSm) - 1, '_');

		//printBufferContents(bufferSm, " initial buffer");

		debug_resource default_dbg{ "default" };
		std::pmr::monotonic_buffer_resource monotonic{ std::data(bufferSm), std::size(bufferSm), &default_dbg };
		debug_resource dbg{ "monotonic", &monotonic };

		std::pmr::vector < std::string > strings{ &dbg };

		strings.emplace_back("alpha");
		strings.emplace_back("beta");
		strings.emplace_back("ZZ");
		strings.emplace_back("Hello");
		strings.emplace_back("CCCCCCCCCCCCCCCCCCCCCCCCCCC");

		printBufferContents(std::string_view{ bufferSm, BUF_SIZE }, " after insertion --> STD::strings"); // ?????
	}

	{
		LOG_TESTCASE("debug resource - pmr::string");

		constexpr size_t BUF_SIZE = 1024; // GOTCHA::: 256 --> too small, calls default allocator!
		char bufferSm[BUF_SIZE] = {}; // a buffer on the stack
		std::fill_n(std::begin(bufferSm), std::size(bufferSm) - 1, '_');

		debug_resource default_dbg{ "default" };
		std::pmr::monotonic_buffer_resource monotonic{ std::data(bufferSm), std::size(bufferSm), &default_dbg };
		debug_resource dbg{ "monotonic", &monotonic };

		std::pmr::vector < std::pmr::string > strings{ &dbg };

		strings.emplace_back("alpha");
		strings.emplace_back("beta");
		strings.emplace_back("ZZ");
		strings.emplace_back("Hello");
		strings.emplace_back("CCCCCCCCCCCCCCCCCCCCCCCCCCC");

		std::pmr::string s(&dbg);
		s.append("SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS"); // OPEN TODO:::: ????
		s.append("LOGO");

		printBufferContents(std::string_view{ bufferSm, BUF_SIZE }, " after insertion --> PMR::strings"); // ?????
	}

	// not an AA-type
	{
		LOG_TESTCASE("debug resource - non-AA/PMR class");

		constexpr size_t BUF_SIZE = 256;
		char buffer[BUF_SIZE] = {}; // a small buffer on the stack
		std::fill_n(std::begin(buffer), std::size(buffer) - 1, '_');

		debug_resource default_dbg{ "default" };
		std::pmr::monotonic_buffer_resource pool{ std::data(buffer), std::size(buffer), &default_dbg };
		debug_resource dbg{ "buffer", &pool };
		std::pmr::vector<SomeProduct> products{ &dbg };
		products.reserve(3);

		products.emplace_back(SomeProduct{ "car", '7' });
		products.emplace_back(SomeProduct{ "TV", '9' });
		products.emplace_back(SomeProduct{ "a bit longer product name", '4' });
		//products[0] = products[2];

		/*SomeProduct p1 { "a bit longer product name", '?', &dbg };
		SomeProduct p2 { &default_dbg };
		p2 = std::move(p1);
		std::cout << p1._name << '\n';*/

		printBufferContents(std::string_view{ buffer, BUF_SIZE }, " after insertions --> SomeProduct");
	}

	// AA-type (i.e Allocator Aware)
	{
		LOG_TESTCASE("debug resource - AA-class with PMRs");

		constexpr size_t BUF_SIZE = 256;
		char buffer[BUF_SIZE] = {}; // a small buffer on the stack
		std::fill_n(std::begin(buffer), std::size(buffer) - 1, '_');

		debug_resource default_dbg{ "default" };
		std::pmr::monotonic_buffer_resource pool{ std::data(buffer), std::size(buffer), &default_dbg };
		debug_resource dbg{ "buffer", &pool };
		std::pmr::vector<SomeProduct_Pmr> products{ &dbg };
		products.reserve(3);

		products.emplace_back(SomeProduct_Pmr{ "car", '7', &dbg });
		products.emplace_back(SomeProduct_Pmr{ "TV", '9', &dbg });
		products.emplace_back(SomeProduct_Pmr{ "a bit longer product name", '4', &dbg });
		//products[0] = products[2];

		/*SomeProduct_Pmr p1 { "a bit longer product name", '?', &dbg };
		SomeProduct_Pmr p2 { &default_dbg };
		p2 = std::move(p1);
		std::cout << p1._name << '\n';*/

		printBufferContents(std::string_view{ buffer, BUF_SIZE }, " after insertions --> SomeProduct_Pmrs");
	}

}

