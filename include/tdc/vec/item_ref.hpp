#pragma once

namespace tdc {
namespace vec {

/// \brief Proxy for reading and writing a single item in a vector.
/// \tparam vector_t the vector type
/// \tparam item_t the item type
template<typename vector_t, typename item_t>
class ItemRef {
private:
	vector_t* m_vec;
	size_t m_i;
	
public:
	/// \brief Main constructor.
	/// \param vec the vector this proxy belongs to.
	/// \param i the number of the referred item.
	inline ItemRef(vector_t& vec, size_t i) : m_vec(&vec), m_i(i) {
	}

	/// \brief Reads the referred item.
	inline operator item_t() const {
		return m_vec->get(m_i);
	}

	/// \brief Writes the referred integer.
	/// \param v the value to write
	inline void operator=(const item_t v) {
		m_vec->set(m_i, v);
	}
};

}} // namespace tdc::vec
