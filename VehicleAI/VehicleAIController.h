#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Engine/Engine.h"
#include "Components/SplineComponent.h"
#include "Components/AudioComponent.h"
#include "TimerManager.h"
#include "VehicleAIController.generated.h"

/**
 * Trafik ışığı durumlarını temsil eden enum.
 * Trafik ışığı sisteminde kullanılacak durumları tanımlar.
 */
UENUM(BlueprintType)
enum class ETrafficLightState : uint8
{
	Red		UMETA(DisplayName = "Red"),
	Yellow	UMETA(DisplayName = "Yellow"),
	Green	UMETA(DisplayName = "Green")
};

/**
 * Araç davranış durumlarını temsil eden enum.
 * Araçların mevcut durumunu belirler (normal sürüş, bekleme, şerit değiştirme).
 */
UENUM(BlueprintType)
enum class EVehicleBehavior : uint8
{
	Normal		UMETA(DisplayName = "Normal"),
	Waiting		UMETA(DisplayName = "Waiting"),
	LaneChanging	UMETA(DisplayName = "Lane Changing")
};

/**
 * Araç AI kontrolcüsü sınıfı.
 * AAIController'dan türeyen bu sınıf, araçların otomatik kontrolü için
 * algılama, kinematik hesaplamalar ve trafik ışığı yönetimi sağlar.
 */
UCLASS()
class YOURGAMENAME_API AVehicleAIController : public AAIController
{
	GENERATED_BODY()

public:
	AVehicleAIController(const FObjectInitializer& ObjectInitializer);

	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	// ============================================
	// ALGILAMA (PERCEPTION) DEĞİŞKENLERİ
	// ============================================
	
	/**
	 * Önündeki engelleri algılamak için kullanılan maksimum algılama mesafesi.
	 * Öklid mesafesi (Euclidean Distance) kullanılarak aracın önündeki nesnelerin
	 * uzaklığını ölçmek için kullanılır. LineTrace işlemlerinde bu mesafe
	 * içindeki nesneler algılanır.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception", meta = (ClampMin = "0.0"))
	float DetectionDistance;

	/**
	 * Engelin rotamızda olup olmadığını anlamak için kullanılan eşik değeri.
	 * Dot Product (nokta çarpım) hesaplamasında kullanılır. Araç yön vektörü ile
	 * engel yön vektörü arasındaki açıyı kontrol eder. Değer 1.0'a yakınsa engel
	 * tam önümüzde, 0.0'a yakınsa yanımızda demektir.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float DotProductThreshold;

	// ============================================
	// KİNEMATİK DEĞİŞKENLER
	// ============================================
	
	/**
	 * Aracın mevcut hızı (m/s veya cm/s).
	 * Kinematik hesaplamalarda kullanılır. Durma mesafesi (braking distance)
	 * hesaplamalarında v² = u² + 2as formülünde başlangıç hızı (u) olarak kullanılır.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinematics", meta = (ClampMin = "0.0"))
	float CurrentSpeed;

	/**
	 * Aracın hedeflenen hızı (m/s veya cm/s).
	 * SmoothSpeedTransition fonksiyonunda Lerp işlemi ile CurrentSpeed'e
	 * yaklaştırılır. Trafik ışığı durumuna veya engel durumuna göre ayarlanır.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinematics", meta = (ClampMin = "0.0"))
	float TargetSpeed;

	/**
	 * Aracın maksimum hızı (m/s veya cm/s).
	 * Yeşil ışıkta veya engel yokken TargetSpeed bu değere ayarlanır.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinematics", meta = (ClampMin = "0.0"))
	float MaxSpeed;

	/**
	 * Maksimum frenleme gücü (negatif ivme, m/s² veya cm/s²).
	 * Kinematik formül: v² = u² + 2as
	 * Burada 'a' negatif değer (frenleme) olarak kullanılır.
	 * CalculateBrakingDistance fonksiyonunda durma mesafesi hesaplamak için kullanılır.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinematics", meta = (ClampMax = "0.0"))
	float MaxBrakingDeceleration;

	// ============================================
	// TRAFİK IŞIĞI DURUMU
	// ============================================
	
	/**
	 * Aracın o anki trafik ışığı durumu.
	 * Trafik ışığı sisteminden gelen bilgiyi tutar ve
	 * buna göre TargetSpeed değeri ayarlanır.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic Light")
	ETrafficLightState CurrentTrafficLightState;

	// ============================================
	// SPLINE TAKİP
	// ============================================

	/**
	 * Aracın takip edeceği yolu temsil eden spline component referansı.
	 * Editor'da veya runtime'da atanabilir.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Path")
	USplineComponent* TargetSpline;

	/**
	 * Takip mesafesi (birim).
	 * Aracın önündeki hangi noktaya bakacağını belirler.
	 * Spline üzerinde en yakın noktadan itibaren bu kadar ileriye bakılır.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Path", meta = (ClampMin = "1.0"))
	float LookAheadDistance;

	/**
	 * UpdateSteering() tarafından hesaplanan güncel direksiyon değeri.
	 * -1.0 = tam sol, 0.0 = düz, 1.0 = tam sağ.
	 * Vehicle sınıfı ApplySteering(CurrentSteerValue) için bu değeri kullanır.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Steering")
	float CurrentSteerValue;

	// ============================================
	// ŞERİT DEĞİŞTİRME
	// ============================================

	/**
	 * Aracın mevcut davranış durumu.
	 * Normal: Normal sürüş modu
	 * Waiting: Bekleme modu (trafik ışığı, engel vb.)
	 * LaneChanging: Şerit değiştirme modu
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lane Changing")
	EVehicleBehavior CurrentVehicleBehavior;

	/**
	 * Spline üzerindeki mevcut şerit offset değeri (birim).
	 * Pozitif değer = sağa kayma, Negatif değer = sola kayma.
	 * UpdateSteering() fonksiyonunda hedef noktayı kaydırmak için kullanılır.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Lane Changing")
	float CurrentLaneOffset;

	/**
	 * Spline üzerindeki hedef şerit offset değeri (birim).
	 * CurrentLaneOffset bu değere doğru yumuşakça yaklaşır.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lane Changing")
	float TargetLaneOffset;

	/**
	 * Şerit değiştirme hızı (birim/saniye).
	 * CurrentLaneOffset'in TargetLaneOffset'e ne kadar hızlı yaklaşacağını belirler.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lane Changing", meta = (ClampMin = "0.0"))
	float LaneChangeSpeed;

	// ============================================
	// PANİK SİSTEMİ
	// ============================================

	/**
	 * Panik durumu flag'i.
	 * Eğer true ise, CheckForwardPath fonksiyonu trafik ışıklarını (TrafficLight) görmezden gelir.
	 * Panik modunda araç trafik ışıklarına bakmaksızın maksimum hızda devam eder.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Panic System")
	bool bIsPanicking;

	/**
	 * Panik modunu kapatmak için kullanılan timer handle.
	 * OnWeaponFireDetected() çağrıldığında 10 saniye sonra panik modunu kapatır.
	 */
	FTimerHandle PanicTimerHandle;

	// ============================================
	// HIZ SABİTLEME (ACC - ADAPTIVE CRUISE CONTROL)
	// ============================================

	/**
	 * Güvenli takip mesafesi (birim).
	 * Öndeki araçla mesafe bu değerden azsa, hızı öndeki aracın hızına eşitler.
	 * ACC (Adaptive Cruise Control) sistemi için kullanılır.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACC", meta = (ClampMin = "0.0"))
	float SafeFollowingDistance;

	// ============================================
	// KORNA SİSTEMİ
	// ============================================

	/**
	 * Korna sesi için kullanılan audio component.
	 * PlayHorn() fonksiyonu bu component üzerinden ses çalar.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Horn System")
	class UAudioComponent* HornAudioComponent;

public:
	// ============================================
	// FONKSİYONLAR
	// ============================================

	/**
	 * Mevcut hıza göre duruş mesafesini hesaplayan fonksiyon.
	 * 
	 * Kinematik formül kullanılır:
	 * v² = u² + 2as
	 * 
	 * Durma durumunda:
	 * 0² = CurrentSpeed² + 2 * MaxBrakingDeceleration * s
	 * s = -(CurrentSpeed²) / (2 * MaxBrakingDeceleration)
	 * 
	 * @return Hesaplanan duruş mesafesi (cm veya m)
	 */
	UFUNCTION(BlueprintCallable, Category = "Kinematics")
	float CalculateBrakingDistance() const;

	/**
	 * LineTrace (ışın atma) ve Dot Product (nokta çarpım) kullanarak
	 * önündeki engelleri kontrol eden fonksiyon.
	 * 
	 * Matematiksel yaklaşım:
	 * 1. LineTrace ile DetectionDistance mesafesinde nesne algılama
	 * 2. Dot Product ile engelin rotamızda olup olmadığını kontrol:
	 *    DotProduct = ForwardVector · ToObstacleVector
	 *    Eğer DotProduct > DotProductThreshold ise engel rotamızda
	 * 3. Eğer engel bir ATrafficLight ise ve durumu Red veya Yellow ise,
	 *    TargetSpeed'i 0'a çeker
	 * 
	 * @param OutHitResult LineTrace sonucunu döndüren parametre
	 * @return Engelin algılanıp algılanmadığı (true = engel var)
	 */
	UFUNCTION(BlueprintCallable, Category = "Perception")
	bool CheckForwardPath(FHitResult& OutHitResult);

	/**
	 * Hızı hedef hıza (0 veya max) yaklaştıran Lerp tabanlı fonksiyon.
	 * 
	 * Linear Interpolation (Lerp) formülü:
	 * NewSpeed = CurrentSpeed + (TargetSpeed - CurrentSpeed) * Alpha
	 * 
	 * Alpha değeri genellikle DeltaTime ve bir hız geçiş katsayısı ile
	 * hesaplanır. Bu sayede hız değişimi yumuşak ve doğal olur.
	 * 
	 * @param DeltaTime Frame süresi (saniye)
	 * @param TransitionSpeed Hız geçiş katsayısı (ne kadar hızlı geçiş yapılacağı)
	 * @return Yeni hız değeri
	 */
	UFUNCTION(BlueprintCallable, Category = "Kinematics")
	float SmoothSpeedTransition(float DeltaTime, float TransitionSpeed = 5.0f);

	/**
	 * Spline üzerindeki hedef noktaya göre direksiyon değerini günceller.
	 * En yakın noktadan LookAheadDistance kadar ilerideki noktayı hedef alır.
	 * Hedef yön (TargetDirection) ile aracın RightVector'ü Dot Product yapılarak
	 * ne kadar sağa/sola kırılacağı hesaplanır.
	 * CurrentLaneOffset kullanılarak hedef nokta spline'a dik yönde kaydırılır.
	 *
	 * @return -1.0 ile 1.0 arası direksiyon değeri (negatif = sol, pozitif = sağ)
	 */
	UFUNCTION(BlueprintCallable, Category = "Steering")
	float UpdateSteering();

	/**
	 * Yan taraftaki yolun açık olup olmadığını kontrol eden fonksiyon.
	 * Şerit değiştirme öncesi güvenlik kontrolü için kullanılır.
	 * 
	 * @param bCheckRight true = sağ tarafı kontrol et, false = sol tarafı kontrol et
	 * @return true = yol açık (şerit değiştirilebilir), false = yol kapalı (engel var)
	 */
	UFUNCTION(BlueprintCallable, Category = "Lane Changing")
	bool IsSidePathClear(bool bCheckRight);

	/**
	 * Korna sesini çalan fonksiyon.
	 * UAudioComponent üzerinden ses çalar.
	 * Şimdilik sadece tanım olarak bırakılmıştır.
	 */
	UFUNCTION(BlueprintCallable, Category = "Horn System")
	void PlayHorn();

	/**
	 * Silah ateşi algılandığında çağrılan fonksiyon.
	 * Panik modunu aktif eder ve 10 saniye sonra otomatik olarak kapatır.
	 * Dış etki (damage) sisteminde kullanılır.
	 */
	UFUNCTION(BlueprintCallable, Category = "Panic System")
	void OnWeaponFireDetected();

private:
	/**
	 * Panik modunu kapatmak için kullanılan private fonksiyon.
	 * Timer tarafından 10 saniye sonra çağrılır.
	 */
	void DisablePanicMode();
};
