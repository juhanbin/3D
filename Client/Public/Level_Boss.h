#pragma once
#include "Client_Defines.h"
#include "Level.h"

NS_BEGIN(Engine)
class CGameObject;
NS_END

NS_BEGIN(Client)

class CBoss_Hand_L;
class CBoss_Hand_R;
class CBoss_Mask;

class CLevel_Boss final : public CLevel {
private:
    CLevel_Boss(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CLevel_Boss() = default;

public:
    HRESULT Initialize() override;
    void    Update(_float fTimeDelta) override;
    HRESULT Render() override;

private:
    // 풀
    void                 Pool_Initialize();
    Engine::CGameObject* CreateBossFire_ForPool();              // ★ 선언 꼭 필요
    Engine::CGameObject* CreateSpear_ForPool();
    // 씬 구성
    HRESULT Ready_Lights();
    HRESULT Ready_Layer_Camera(const _wstring& strLayerTag);
    HRESULT Ready_Layer_Camera_Player(const _wstring& strLayerTag);
    HRESULT Ready_Layer_BackGround(const _wstring& strLayerTag);
    HRESULT Ready_Layer_Player(const _wstring& strLayerTag);
    HRESULT Ready_Layer_Boss_Hand_L(const _wstring& strLayerTag);
    HRESULT Ready_Layer_Boss_Hand_R(const _wstring& strLayerTag);
    HRESULT Ready_Layer_Boss_Mask(const _wstring& strLayerTag);
    HRESULT Ready_Layer_Boss_Controller(const _wstring& strLayerTag); // ★ 선언 꼭 필요
    HRESULT Ready_Layer_MapObjects(const _wstring& strLayerTag);
    HRESULT Ready_Layer_Effect(const _wstring& strLayerTag);

    // 배치 파일
    std::vector<MapObject> m_SceneObjects;
    vector<MapObject> LoadSceneObjects(const char* file);

    // ★ 보스 파츠 포인터
    CBoss_Hand_L* m_pHandL = nullptr;
    CBoss_Hand_R* m_pHandR = nullptr;
    CBoss_Mask* m_pMask = nullptr;

public:
    static CLevel_Boss* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    void Free() override;
};

NS_END
