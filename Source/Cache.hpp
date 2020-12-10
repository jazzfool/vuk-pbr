#pragma once

#include <unordered_map>
#include <optional>

template <typename K, typename V>
class Cache;

template <typename K, typename V>
struct CacheView {
	CacheView() {
	}

  private:
	CacheView(std::size_t key) : key{key} {
	}

	friend class Cache<K, V>;
	std::optional<std::size_t> key;
};

template <typename K, typename V>
class Cache {
  public:
	static_assert(std::is_copy_constructible_v<K>);
	static_assert(std::is_copy_assignable_v<K>);

	using View = CacheView<K, V>;

	static CacheView<K, V> view(K key);
	static CacheView<K, V> view(std::size_t key);

	CacheView<K, V> insert(K key, V&& value);
	CacheView<K, V> insert(std::size_t key, V&& value);

	bool valid(const CacheView<K, V>& view) const;

	V& get(const CacheView<K, V>& view);
	const V& get(const CacheView<K, V>& view) const;

  private:
	std::unordered_map<std::size_t, V> m_cache;
};

template <typename K, typename V>
CacheView<K, V> Cache<K, V>::view(K key) {
	return CacheView<K, V>{(std::hash<K>{})(key)};
}

template <typename K, typename V>
CacheView<K, V> Cache<K, V>::view(std::size_t key) {
	return CacheView<K, V>{key};
}

template <typename K, typename V>
CacheView<K, V> Cache<K, V>::insert(K key, V&& value) {
	auto hash = (std::hash<K>{})(key);
	m_cache.emplace(hash, std::move(value));
	return CacheView<K, V>{hash};
}

template <typename K, typename V>
CacheView<K, V> Cache<K, V>::insert(std::size_t key, V&& value) {
	m_cache.emplace(key, std::move(value));
	return CacheView<K, V>{key};
}

template <typename K, typename V>
bool Cache<K, V>::valid(const CacheView<K, V>& view) const {
	if (!view.key.has_value())
		return false;
	return m_cache.contains(view.key);
}

template <typename K, typename V>
V& Cache<K, V>::get(const CacheView<K, V>& view) {
	return m_cache.at(*view.key);
}

template <typename K, typename V>
const V& Cache<K, V>::get(const CacheView<K, V>& view) const {
	return m_cache.at(*view.key);
}
