#pragma once

#include "Level.h"
#include <fstream>

class CEdit final : public CLevel
{
private:
    CEdit(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CEdit() = default;

public:
    virtual HRESULT Initialize() override;
    virtual void Update(_float fTimeDelta) override;
    virtual HRESULT Render() override;

private:
    HRESULT Ready_Lights();
    HRESULT Ready_Layer_Camera(const _wstring& strLayerTag);
    HRESULT Ready_Layer_MapObjects(const _wstring& strLayerTag);

public:
    static CEdit* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual void Free() override;
};
