#pragma once

namespace fast_io
{

namespace stream_view_details
{

template<typename T>
struct streambuf_pub:public T
{
public:
	using T::showmanyc;
	using T::setg;
	using T::eback;
	using T::gptr;
	using T::egptr;
	using T::underflow;

	using T::pbase;
	using T::pptr;
	using T::epptr;
	using T::overflow;
	using T::pbump;
	using T::setp;
};

}

template<stream_view_details::streambuf_concept_impl T>
class streambuf_view
{
	stream_view_details::streambuf_pub<T>* rdb;
public:
	using char_type = typename T::char_type;
	using traits_type = typename T::traits_type;
//be aware of type confusion security vulnerability
	streambuf_view(T* bufpointer):
		rdb(bit_cast<stream_view_details::streambuf_pub<T>*>(bufpointer)){}
	inline T* rdbuf() const
	{
		return rdb;
	}
	inline auto native_handle() const
	{
		return rdb;
	}
};

template<typename T,std::contiguous_iterator Iter>
inline Iter read(streambuf_view<T>& t,Iter begin,Iter end)
{
	using char_type = typename T::char_type;
	return begin+(t.native_handle()->sgetn(static_cast<char_type*>(static_cast<void*>(std::to_address(begin))),(end-begin)*sizeof(*begin)/sizeof(char_type))*sizeof(char_type)/sizeof(*begin));
}

template<bool err=false,typename T>
inline auto get(streambuf_view<T>& t)
{
	using traits_type = typename T::traits_type;
	auto ch(t.native_handle()->sbumpc());
	if(ch==traits_type::eof())
	{
		if constexpr(err)
			return std::pair<typename T::char_type,bool>{0,true};
		else
			throw fast_io::eof();
	}
	if constexpr(err)
		return std::pair{traits_type::to_char_type(ch),false};
	else
		return traits_type::to_char_type(ch);
}

template<typename T>
inline void put(streambuf_view<T>& t,typename T::char_type ch)
{
	if(!t.native_handle()->sputc(ch))
#ifdef __cpp_exceptions
		throw std::system_error(std::make_error_code(std::errc::io_error));
#else
		fast_terminate();
#endif
}

template<typename T,std::contiguous_iterator Iter>
inline void write(streambuf_view<T>& t,Iter begin,Iter end) 
{
	using char_type = typename T::char_type;
	if(!t.native_handle()->sputn(static_cast<char_type const*>(static_cast<void const*>(std::to_address(begin))),(end-begin)*sizeof(*begin)/sizeof(char_type)))
#ifdef __cpp_exceptions
		throw std::system_error(std::make_error_code(std::errc::io_error));
#else
		fast_terminate();
#endif

}

template<typename T>
inline const void flush(streambuf_view<T>& t)
{
//	t.native_handle()->overflow();
}

template<typename T>
inline auto oreserve(streambuf_view<T>& t,std::size_t size) -> typename T::char_type*
{
	auto& nh(*t.native_handle());
	auto p(nh.pptr());
	if(static_cast<std::size_t>(nh.epptr()-p)<=size)
		return nullptr;
	nh.pbump(static_cast<int>(size));
	return p+size;
}

template<typename T>
inline void orelease(streambuf_view<T>& t,std::size_t size)
{
	t.native_handle()->pbump(-static_cast<int>(size));
}

template<stream_view_details::streambuf_concept_impl streambf>
streambuf_view(streambf*) -> streambuf_view<streambf>;


}