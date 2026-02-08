#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "VehicleAIController.h" // ETrafficLightState enum'u için
#include "TrafficLight.generated.h"

/**
 * Trafik ışığı aktörü.
 * AActor'dan türeyen bu sınıf, trafik ışıklarının durumunu yönetir ve
 * araçların bu ışıkları algılamasını sağlar.
 */
UCLASS()
class YOURGAMENAME_API ATrafficLight : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATrafficLight();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// ============================================
	// TRAFİK IŞIĞI DURUMU
	// ============================================
	
	/**
	 * Trafik ışığının mevcut durumu.
	 * ETrafficLightState enum'u VehicleAIController.h dosyasında tanımlıdır.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic Light")
	ETrafficLightState CurrentState;

	// ============================================
	// SÜRE DEĞİŞKENLERİ
	// ============================================
	
	/**
	 * Yeşil ışığın süresi (saniye).
	 * Bu süre sonunda ışık sarıya geçer.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic Light Timing", meta = (ClampMin = "0.1"))
	float GreenDuration;

	/**
	 * Sarı ışığın süresi (saniye).
	 * Bu süre sonunda ışık kırmızıya geçer.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic Light Timing", meta = (ClampMin = "0.1"))
	float YellowDuration;

	/**
	 * Kırmızı ışığın süresi (saniye).
	 * Bu süre sonunda ışık yeşile geçer.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic Light Timing", meta = (ClampMin = "0.1"))
	float RedDuration;

	// ============================================
	// COMPONENT'LER
	// ============================================
	
	/**
	 * Trigger Box Component.
	 * Araçların bu trafik ışığını algılaması için kullanılır.
	 * OnComponentBeginOverlap ve OnComponentEndOverlap event'leri ile
	 * araçların giriş/çıkışını takip edebiliriz.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* TriggerBox;

	// ============================================
	// TIMER
	// ============================================
	
	/**
	 * Işık değişimi için kullanılan timer handle.
	 * FTimerHandle, Unreal Engine'in timer sistemini kullanarak
	 * belirli aralıklarla SwitchLight() fonksiyonunu çağırmamızı sağlar.
	 */
	FTimerHandle LightSwitchTimerHandle;

	// Tick fonksiyonu kullanılmıyor (performans için kapatıldı)
	// virtual void Tick(float DeltaTime) override;

	// ============================================
	// FONKSİYONLAR
	// ============================================

	/**
	 * Işığın durumunu değiştiren fonksiyon.
	 * Mevcut duruma göre bir sonraki duruma geçer:
	 * Green -> Yellow -> Red -> Green (döngüsel)
	 * 
	 * FTimerHandle kullanılarak otomatik olarak çağrılır.
	 */
	UFUNCTION(BlueprintCallable, Category = "Traffic Light")
	void SwitchLight();

	/**
	 * AI Controller'ın trafik ışığının mevcut durumunu sorgulaması için fonksiyon.
	 * 
	 * @return Mevcut trafik ışığı durumu (ETrafficLightState)
	 */
	UFUNCTION(BlueprintCallable, Category = "Traffic Light")
	ETrafficLightState GetCurrentState() const;

	/**
	 * Trafik ışığını belirli bir duruma ayarlayan fonksiyon.
	 * 
	 * @param NewState Ayarlanacak yeni durum
	 */
	UFUNCTION(BlueprintCallable, Category = "Traffic Light")
	void SetLightState(ETrafficLightState NewState);

	/**
	 * Trigger Box component'ine erişim sağlar.
	 * 
	 * @return Trigger Box component referansı
	 */
	UFUNCTION(BlueprintCallable, Category = "Components")
	UBoxComponent* GetTriggerBox() const { return TriggerBox; }
};
