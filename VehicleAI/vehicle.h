#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "VehicleAIController.h"
#include "Vehicle.generated.h"

/**
 * Araç sınıfı.
 * APawn'dan türeyen bu sınıf, araçların görsel temsilini ve hareket mekanizmasını sağlar.
 * AI Controller ile entegre çalışarak otomatik sürüş özelliği sunar.
 */
UCLASS()
class YOURGAMENAME_API AVehicle : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AVehicle(const FObjectInitializer& ObjectInitializer);

	/**
	 * AI Controller Class.
	 * Araç spawn olduğunda otomatik olarak AVehicleAIController yüklenir.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TSubclassOf<AVehicleAIController> AIControllerClass;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// ============================================
	// COMPONENT'LER
	// ============================================

	/**
	 * Araç modeli için kullanılan skeletal mesh component.
	 * Araç görsel modelini temsil eder.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* VehicleMesh;

	/**
	 * Spring Arm Component.
	 * Kamera için esnek bir bağlantı sağlar ve kamera çarpışmalarını yönetir.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArm;

	/**
	 * Camera Component.
	 * Oyuncunun veya debug için araç görüntüsünü sağlar.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* Camera;

	// ============================================
	// AI CONTROLLER REFERANSI
	// ============================================

	/**
	 * AI Controller referansı.
	 * Araç ve AI Controller arasında sürekli iletişim için kullanılır.
	 * BeginPlay'de otomatik olarak ayarlanır.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	AVehicleAIController* VehicleAIControllerRef;

	// ============================================
	// HAREKET PARAMETRELERİ
	// ============================================

	/**
	 * Maksimum hız (cm/s veya m/s).
	 * ApplyMovement fonksiyonunda kullanılır.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "0.0"))
	float MaxMovementSpeed;

	/**
	 * Maksimum direksiyon açısı (derece).
	 * ApplySteering fonksiyonunda kullanılır.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float MaxSteeringAngle;

	/**
	 * Hareket gücü çarpanı.
	 * Hızın ne kadar güçlü uygulanacağını belirler.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "0.0"))
	float MovementForceMultiplier;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// ============================================
	// HAREKET FONKSİYONLARI
	// ============================================

	/**
	 * AI Controller'dan gelen hız değerine göre aracı hareket ettiren fonksiyon.
	 * 
	 * @param Speed Hareket hızı (0.0 - 1.0 arası normalize edilmiş değer veya gerçek hız)
	 */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void ApplyMovement(float Speed);

	/**
	 * AI Controller'dan gelen direksiyon değerine göre aracı yönlendiren fonksiyon.
	 * 
	 * @param SteerValue Direksiyon değeri (-1.0 ile 1.0 arası: -1.0 = sol, 0.0 = düz, 1.0 = sağ)
	 */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void ApplySteering(float SteerValue);

	// ============================================
	// GETTER FONKSİYONLARI
	// ============================================

	/**
	 * AI Controller referansını döndürür.
	 * 
	 * @return AI Controller referansı (nullptr olabilir)
	 */
	UFUNCTION(BlueprintCallable, Category = "AI")
	AVehicleAIController* GetVehicleAIController() const { return VehicleAIControllerRef; }

	/**
	 * Vehicle Mesh component'ine erişim sağlar.
	 * 
	 * @return Vehicle Mesh component referansı
	 */
	UFUNCTION(BlueprintCallable, Category = "Components")
	USkeletalMeshComponent* GetVehicleMesh() const { return VehicleMesh; }
};
