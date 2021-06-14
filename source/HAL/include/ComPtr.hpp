#pragma once

#include <cassert>

namespace HAL {
    template<typename T>
    struct ComPtr {
        ComPtr(T* pComObj = nullptr) : m_pComObj(pComObj) {
            static_assert(std::is_base_of<IUnknown, T>::value, "T needs to be IUnknown based");
            if (m_pComObj)
                m_pComObj->AddRef();
        }

        ComPtr(const ComPtr<T>& pComObj) {
            static_assert(std::is_base_of<IUnknown, T>::value, "T needs to be IUnknown based");
            m_pComObj = pComObj.m_pComObj;
            if (m_pComObj)
                m_pComObj->AddRef();
        }

        ComPtr(ComPtr<T>&& pComObj) {
            m_pComObj = pComObj.m_pComObj;
            pComObj.m_pComObj = nullptr;
        }

        T* operator=(T* pComObj) {
            if (m_pComObj)
                m_pComObj->Release();

            m_pComObj = pComObj;

            if (m_pComObj)
                m_pComObj->AddRef();

            return m_pComObj;
        }

        T* operator=(const ComPtr<T>& pComObj) {
            if (m_pComObj)
                m_pComObj->Release();

            m_pComObj = pComObj.m_pComObj;

            if (m_pComObj)
                m_pComObj->AddRef();

            return m_pComObj;
        }

        ~ComPtr() {
            if (m_pComObj) {
                m_pComObj->Release();
                m_pComObj = nullptr;
            }
        }

        operator T* () const {
            return m_pComObj;
        }

        T& operator*() const {
            return *m_pComObj;
        }

        T** operator&() {
            assert(nullptr == m_pComObj);
            return &m_pComObj;
        }

        T* operator->() const {
            return m_pComObj;
        }

        bool operator!() const {
            return (nullptr == m_pComObj);
        }

        bool operator<(T* pComObj) const {
            return m_pComObj < pComObj;
        }

        bool operator!=(T* pComObj) const {
            return !operator==(pComObj);
        }

        bool operator==(T* pComObj) const {
            return m_pComObj == pComObj;
        }

        template <typename I>
        HRESULT QueryInterface(I** ppInterface) {
            return m_pComObj->QueryInterface(IID_PPV_ARGS(ppInterface));
        }

    protected:
        T* m_pComObj;
    };

    class ComException : public std::exception {
	public:
		ComException(HRESULT hr) : m_Result(hr) {}

		const char* what() const override {
			static char s_str[64] = {};
			sprintf_s(s_str, "Failure with HRESULT of %08X" ,static_cast<uint32_t>(m_Result));
			return s_str;
		}
	private:
		HRESULT m_Result;
	};

	inline auto ThrowIfFailed(HRESULT hr) -> void {
		if (FAILED(hr))	
			throw ComException(hr);
	}
}