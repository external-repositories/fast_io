#pragma once

#include <variant>

namespace fast_io
{

struct dns_iterator
{
	addrinfo *ptr;
};
inline constexpr bool operator==(dns_iterator const& a, dns_iterator const& b)
{
	return a.ptr == b.ptr;
}
inline constexpr bool operator!=(dns_iterator const& a, dns_iterator const& b)
{
	return !(a==b);
}
inline constexpr bool operator==(std::default_sentinel_t, dns_iterator const& b)
{
	return b.ptr == nullptr;
}
inline constexpr bool operator==(dns_iterator const& b, std::default_sentinel_t)
{
	return b.ptr == nullptr;
}
inline constexpr bool operator!=(std::default_sentinel_t, dns_iterator const& b)
{
	return b.ptr != nullptr;
}
inline constexpr bool operator!=(dns_iterator const& b, std::default_sentinel_t)
{
	return b.ptr != nullptr;
}
inline address operator*(dns_iterator const &a)
{
	if (a.ptr->ai_family == AF_INET)
	{
		sockaddr_in addr;
		memcpy(std::addressof(addr), a.ptr->ai_addr, sizeof(addr));
		ipv4 ret;
		memcpy(ret.storage.data(), std::addressof(addr.sin_addr), sizeof(ret.storage));
		return address(ret);
	}
	else if (a.ptr->ai_family == AF_INET6)
	{
		sockaddr_in6 addr;
		memcpy(std::addressof(addr), a.ptr->ai_addr, sizeof(addr));
		ipv6 ret;
		memcpy(ret.storage.data(), std::addressof(addr.sin6_addr), sizeof(ret.storage));
		if constexpr(std::endian::native==std::endian::little)
			for(auto& e : ret.storage)
				e=details::byte_swap(e);
		return address(ret);
	}
#ifdef __cpp_exceptions
	throw fast_io_text_error("unknown family");
#else
	fast_terminate();
#endif
}
inline constexpr dns_iterator& operator++(dns_iterator& a)
{
	a.ptr = a.ptr->ai_next;
	return a;
}
inline constexpr dns_iterator operator++(dns_iterator& a, int)
{
	auto temp(a);
	++a;
	return temp;
}

template<fast_io::sock::family fam>
class basic_dns
{
	addrinfo *result;
public:
	basic_dns(std::string_view host)
	{
		addrinfo hints{};
		hints.ai_family = static_cast<fast_io::sock::details::address_family>(fam);
		fast_io::sock::details::getaddrinfo(host.data(), nullptr, std::addressof(hints), std::addressof(result));
	}
	basic_dns(std::u8string_view host):basic_dns({reinterpret_cast<char const*>(host.data()),host.size()})
	{}
	constexpr auto& get_result()
	{
		return result;
	}
	constexpr auto& get_result() const
	{
		return result;
	}
	basic_dns(basic_dns const&) = delete;
	basic_dns& operator=(basic_dns const&) = delete;
	basic_dns(basic_dns&& d) noexcept:result(d.result)
	{
		d.result=nullptr;
	}
	basic_dns& operator=(basic_dns&& d) noexcept
	{
		if(std::addressof(d)!=this)
		{
			if(result)
				fast_io::sock::details::freeaddrinfo(result);
			result=d.result;
			d.result=nullptr;
		}
		return *this;
	}
	~basic_dns()
	{
		if(result)
			fast_io::sock::details::freeaddrinfo(result);
	}
};

using ipv4_dns = basic_dns<fast_io::sock::family::ipv4>;
using ipv6_dns = basic_dns<fast_io::sock::family::ipv6>;
using dns = basic_dns<fast_io::sock::family::unspec>;

template<fast_io::sock::family fam>
inline constexpr auto cbegin(basic_dns<fam> const& d)
{
	return dns_iterator{d.get_result()};
}

template<fast_io::sock::family fam>
inline constexpr auto begin(basic_dns<fam> const& d)
{
	return cbegin(d);
}

template<fast_io::sock::family fam>
inline constexpr auto begin(basic_dns<fam> & d)
{
	return dns_iterator{d.get_result()};
}

template<fast_io::sock::family fam>
inline constexpr std::default_sentinel_t end(basic_dns<fam> const&)
{
	return {};
}

template<fast_io::sock::family fam>
inline constexpr std::default_sentinel_t cend(basic_dns<fam> const&)
{
	return {};
}

template<fast_io::sock::family fam=fast_io::sock::family::unspec>
inline constexpr auto dns_once(std::string_view host)
{
	return *cbegin(basic_dns<fam>(host));
}

template<fast_io::sock::family fam=fast_io::sock::family::unspec>
inline constexpr auto dns_once(std::u8string_view host)
{
	return *cbegin(basic_dns<fam>(host));
}

}