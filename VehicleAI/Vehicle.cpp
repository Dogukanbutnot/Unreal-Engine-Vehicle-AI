#include "Vehicle.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"

// Sets default values
AVehicle::AVehicle(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set this pawn to call Tick() every frame
	PrimaryActorTick.bCanEverTick = true;

	// AI Controller Class'ı ayarla - spawn olduğunda otomatik yüklenecek
	AIControllerClass = AVehicleAIController::StaticClass();

	// Root component oluştur (Scene Component)
	USceneComponent* RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = RootSceneComponent;

	// Vehicle Mesh Component oluştur
	VehicleMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VehicleMesh"));
	VehicleMesh->SetupAttachment(RootComponent);
	VehicleMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	VehicleMesh->SetCollisionObjectType(ECC_Pawn);
	VehicleMesh->SetCollisionResponseToAllChannels(ECR_Block);
	VehicleMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

	// Spring Arm Component oluştur
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 800.0f; // Kamera mesafesi
	SpringArm->bUsePawnControlRotation = false; // Pawn rotasyonunu kullanma
	SpringArm->bInheritPitch = true;
	SpringArm->bInheritYaw = true;
	SpringArm->bInheritRoll = false;
	SpringArm->bDoCollisionTest = true; // Çarpışma testi yap

	// Camera Component oluştur
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false; // Spring Arm rotasyonunu kullan

	// Varsayılan değerler
	MaxMovementSpeed = 1000.0f;
	MaxSteeringAngle = 45.0f;
	MovementForceMultiplier = 1000.0f;
	VehicleAIControllerRef = nullptr;
}

// Called when the game starts or when spawned
void AVehicle::BeginPlay()
{
	Super::BeginPlay();

	// AI Controller referansını al
	// Controller otomatik olarak spawn olduğunda AIControllerClass sayesinde yüklenir
	VehicleAIControllerRef = Cast<AVehicleAIController>(GetController());
	
	if (VehicleAIControllerRef)
	{
		// AI Controller başarıyla bağlandı
		UE_LOG(LogTemp, Log, TEXT("Vehicle AI Controller başarıyla bağlandı"));
	}
	else
	{
		// AI Controller bulunamadı (uyarı ama hata değil - manuel kontrol mümkün)
		UE_LOG(LogTemp, Warning, TEXT("Vehicle AI Controller bulunamadı - manuel kontrol kullanılabilir"));
	}
}

// Called every frame
void AVehicle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// AI Controller'dan hız bilgisini al ve hareket ettir
	if (VehicleAIControllerRef)
	{
		// AI Controller'dan mevcut hızı al
		float CurrentSpeed = VehicleAIControllerRef->CurrentSpeed;
		
		// Normalize edilmiş hız değeri (0.0 - 1.0 arası)
		float NormalizedSpeed = FMath::Clamp(CurrentSpeed / MaxMovementSpeed, 0.0f, 1.0f);
		
		// Hareketi uygula
		ApplyMovement(NormalizedSpeed);

		// Spline takip için UpdateSteering() ile hesaplanan direksiyon değerini uygula
		ApplySteering(VehicleAIControllerRef->CurrentSteerValue);
	}
}

// Called to bind functionality to input
void AVehicle::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	// Manuel kontrol için input binding'ler buraya eklenebilir
	// AI kontrolü kullanıldığında bu fonksiyon genellikle kullanılmaz
}

void AVehicle::ApplyMovement(float Speed)
{
	// Speed değerini clamp et (0.0 - 1.0 arası)
	Speed = FMath::Clamp(Speed, 0.0f, 1.0f);

	// Eğer hız 0 ise hareket etme
	if (FMath::IsNearlyZero(Speed))
	{
		return;
	}

	// Hareket yönünü al (araç yönü)
	FVector ForwardVector = GetActorForwardVector();
	
	// Hızı hesapla (normalize edilmiş değer * maksimum hız)
	float ActualSpeed = Speed * MaxMovementSpeed;
	
	// Hareket vektörünü hesapla
	FVector MovementVector = ForwardVector * ActualSpeed * GetWorld()->GetDeltaSeconds();
	
	// Hareketi uygula
	// Not: Bu basit bir hareket uygulamasıdır
	// Daha gelişmiş fizik için UPawnMovementComponent veya UCharacterMovementComponent kullanılabilir
	AddActorWorldOffset(MovementVector, true);
	
	// Alternatif olarak, fizik tabanlı hareket için:
	// if (VehicleMesh && VehicleMesh->GetBodyInstance())
	// {
	//     FVector Force = ForwardVector * ActualSpeed * MovementForceMultiplier;
	//     VehicleMesh->AddForce(Force);
	// }
}

void AVehicle::ApplySteering(float SteerValue)
{
	// SteerValue değerini clamp et (-1.0 ile 1.0 arası)
	SteerValue = FMath::Clamp(SteerValue, -1.0f, 1.0f);

	// Eğer direksiyon değeri 0 ise düz git
	if (FMath::IsNearlyZero(SteerValue))
	{
		return;
	}

	// Maksimum direksiyon açısını hesapla
	float SteeringAngle = SteerValue * MaxSteeringAngle;
	
	// Yaw rotasyonunu uygula (Y ekseni etrafında dönüş)
	FRotator CurrentRotation = GetActorRotation();
	FRotator NewRotation = CurrentRotation;
	NewRotation.Yaw += SteeringAngle * GetWorld()->GetDeltaSeconds() * 50.0f; // 50.0f = steering speed multiplier
	
	// Rotasyonu uygula
	SetActorRotation(NewRotation);
	
	// Alternatif olarak, daha yumuşak bir rotasyon için:
	// FRotator TargetRotation = CurrentRotation;
	// TargetRotation.Yaw += SteeringAngle;
	// SetActorRotation(FMath::RInterpTo(CurrentRotation, TargetRotation, GetWorld()->GetDeltaSeconds(), 5.0f));
}
