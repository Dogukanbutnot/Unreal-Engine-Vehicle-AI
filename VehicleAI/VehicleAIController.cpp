#include "VehicleAIController.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Math/UnrealMathUtility.h"
#include "CollisionQueryParams.h"
#include "Engine/Engine.h"
#include "TrafficLight.h"
#include "Components/SplineComponent.h"
#include "Components/AudioComponent.h"
#include "TimerManager.h"
#include "Vehicle.h"

AVehicleAIController::AVehicleAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set this controller to call Tick() every frame
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	// Varsayılan değerler
	DetectionDistance = 1000.0f;
	DotProductThreshold = 0.7f;
	CurrentSpeed = 0.0f;
	TargetSpeed = 0.0f;
	MaxSpeed = 1000.0f; // Maksimum hız (cm/s veya m/s)
	MaxBrakingDeceleration = -500.0f; // Negatif değer (frenleme)
	CurrentTrafficLightState = ETrafficLightState::Green;
	TargetSpline = nullptr;
	LookAheadDistance = 500.0f;
	CurrentSteerValue = 0.0f;
	CurrentVehicleBehavior = EVehicleBehavior::Normal;
	CurrentLaneOffset = 0.0f;
	TargetLaneOffset = 0.0f;
	LaneChangeSpeed = 200.0f; // Birim/saniye
	bIsPanicking = false;
	SafeFollowingDistance = 500.0f; // Güvenli takip mesafesi (birim)
	HornAudioComponent = nullptr;
}

void AVehicleAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Ön yol kontrolü: engel / trafik ışığı varsa TargetSpeed güncellenir (Vehicle hızı buna göre uygular)
	FHitResult HitResult;
	CheckForwardPath(HitResult);

	// Hızı hedef hıza doğru yumuşakça yaklaştır (sonuç CurrentSpeed olarak Vehicle'a iletilir)
	SmoothSpeedTransition(DeltaTime);

	// Şerit offset'ini hedef değere doğru yumuşakça yaklaştır
	if (!FMath::IsNearlyEqual(CurrentLaneOffset, TargetLaneOffset))
	{
		float OffsetDelta = FMath::Sign(TargetLaneOffset - CurrentLaneOffset) * LaneChangeSpeed * DeltaTime;
		float NewOffset = CurrentLaneOffset + OffsetDelta;
		
		// Hedef değere ulaşıldıysa clamp et
		if ((TargetLaneOffset > CurrentLaneOffset && NewOffset >= TargetLaneOffset) ||
			(TargetLaneOffset < CurrentLaneOffset && NewOffset <= TargetLaneOffset))
		{
			CurrentLaneOffset = TargetLaneOffset;
		}
		else
		{
			CurrentLaneOffset = NewOffset;
		}

		// Şerit değiştirme durumunu güncelle
		if (FMath::IsNearlyEqual(CurrentLaneOffset, TargetLaneOffset))
		{
			CurrentVehicleBehavior = EVehicleBehavior::Normal;
		}
		else
		{
			CurrentVehicleBehavior = EVehicleBehavior::LaneChanging;
		}
	}

	// Spline takip: direksiyon değerini güncelle (sonuç CurrentSteerValue olarak Vehicle'a iletilir)
	UpdateSteering();
}

float AVehicleAIController::CalculateBrakingDistance() const
{
	// Sıfıra bölme hatasını önlemek için kontrol
	// MaxBrakingDeceleration 0 veya çok küçük bir değerse, duruş mesafesi hesaplanamaz
	if (FMath::IsNearlyZero(MaxBrakingDeceleration))
	{
		return 0.0f;
	}

	// Eğer hız zaten 0 veya çok küçükse, duruş mesafesi 0'dır
	if (FMath::IsNearlyZero(CurrentSpeed))
	{
		return 0.0f;
	}

	// Kinematik formül: s = -(v²) / (2 * a)
	// Burada:
	// v = başlangıç hızı (CurrentSpeed)
	// a = ivme (MaxBrakingDeceleration, negatif değer - frenleme)
	// s = mesafe (braking distance)
	//
	// Formül: s = -(v²) / (2 * a)
	// MaxBrakingDeceleration zaten negatif olduğu için sonuç pozitif olacaktır
	
	float SpeedSquared = CurrentSpeed * CurrentSpeed;
	float Denominator = 2.0f * MaxBrakingDeceleration; // Negatif değer
	
	// Sıfıra bölme kontrolü - MaxBrakingDeceleration 0 veya çok küçükse hata vermemesi için
	if (FMath::IsNearlyZero(MaxBrakingDeceleration))
	{
		return 0.0f;
	}
	
	// Formülü uygula: s = -(v²) / (2 * a)
	float BrakingDistance = -(SpeedSquared / Denominator);
	
	// Negatif değerleri önlemek için (güvenlik kontrolü)
	return FMath::Max(0.0f, BrakingDistance);
}

bool AVehicleAIController::CheckForwardPath(FHitResult& OutHitResult)
{
	// Pawn kontrolü - eğer kontrol edilen bir pawn yoksa false döndür
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return false;
	}

	// World kontrolü - LineTrace için world referansı gerekli
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// Araç yön vektörünü ve konumunu al
	FVector ForwardVector = ControlledPawn->GetActorForwardVector();
	FVector StartLocation = ControlledPawn->GetActorLocation();
	
	// DetectionDistance mesafesinde bir nokta hesapla (Öklid mesafesi)
	FVector EndLocation = StartLocation + (ForwardVector * DetectionDistance);

	// LineTrace parametrelerini ayarla
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(ControlledPawn); // Kendi aracımızı ignore et
	QueryParams.bTraceComplex = false; // Basit collision kullan (performans için)
	QueryParams.bReturnPhysicalMaterial = false; // Fiziksel materyal bilgisine ihtiyacımız yok

	// LineTraceSingleByChannel kullanarak önümüzdeki engelleri tara
	// ECC_Visibility channel'ı görünür nesneleri algılar
	bool bHit = World->LineTraceSingleByChannel(
		OutHitResult,
		StartLocation,
		EndLocation,
		ECC_Visibility, // Collision channel: görünür nesneler
		QueryParams
	);

	// Eğer hiçbir şey algılanmadıysa, engel yok demektir - maksimum hıza dön
	if (!bHit)
	{
		TargetSpeed = MaxSpeed;
		CurrentTrafficLightState = ETrafficLightState::Green;
		return false;
	}

	// Dot Product (nokta çarpım) hesaplama: Engelin rotamızda olup olmadığını kontrol et
	// ForwardVector: Aracın ileri yön vektörü (normalize edilmiş)
	// ToObstacleVector: Aracın konumundan engelin konumuna olan vektör (normalize edilmiş)
	FVector ToObstacleVector = (OutHitResult.ImpactPoint - StartLocation).GetSafeNormal();
	float DotProduct = FVector::DotProduct(ForwardVector, ToObstacleVector);

	// Dot Product açıklaması:
	// - 1.0: Engel tam önümüzde (0 derece açı)
	// - 0.0: Engel yanımızda (90 derece açı)
	// - -1.0: Engel arkamızda (180 derece açı)
	// DotProductThreshold'dan büyükse, engel rotamızda demektir
	// Sadece tam önündeki engeller için dur (DotProductThreshold kontrolü)
	
	bool bObstacleInPath = DotProduct > DotProductThreshold;

	// Eğer engel rotamızda ise (tam önümüzde), trafik ışığı kontrolü yap
	if (bObstacleInPath)
	{
		// LineTrace'in çarptığı aktörü al
		AActor* HitActor = OutHitResult.GetActor();
		
		// Cast<ATrafficLight> ile trafik ışığı olup olmadığını kontrol et
		ATrafficLight* TrafficLight = Cast<ATrafficLight>(HitActor);
		
		if (TrafficLight)
		{
			// Panik sistemi: Eğer panik modundaysa trafik ışıklarını görmezden gel
			if (bIsPanicking)
			{
				// Panik modunda trafik ışığını görmezden gel, maksimum hıza devam et
				TargetSpeed = MaxSpeed;
				CurrentTrafficLightState = ETrafficLightState::Green; // Durumu güncelle
			}
			else
			{
				// Normal mod: Trafik ışığı bulundu, durumunu kontrol et
				ETrafficLightState LightState = TrafficLight->GetCurrentState();
				
				// Eğer trafik ışığı Red veya Yellow ise, TargetSpeed'i 0'a çek
				if (LightState == ETrafficLightState::Red || LightState == ETrafficLightState::Yellow)
				{
					TargetSpeed = 0.0f;
					CurrentTrafficLightState = LightState; // Durumu güncelle
				}
				else if (LightState == ETrafficLightState::Green)
				{
					// Yeşil ışıkta maksimum hıza dön
					TargetSpeed = MaxSpeed;
					CurrentTrafficLightState = LightState; // Durumu güncelle
				}
			}
		}
		else
		{
			// Trafik ışığı değil, engel kontrolü yap
			// ACC (Adaptive Cruise Control): Öndeki araç kontrolü
			AVehicle* FrontVehicle = Cast<AVehicle>(HitActor);
			
			if (FrontVehicle)
			{
				// Öndeki araç ile mesafeyi hesapla
				float DistanceToFrontVehicle = FVector::Dist(StartLocation, OutHitResult.ImpactPoint);
				
				// Güvenli takip mesafesinden azsa, öndeki aracın hızına eşitle
				if (DistanceToFrontVehicle < SafeFollowingDistance)
				{
					// Öndeki aracın AI Controller'ını al
					AVehicleAIController* FrontVehicleController = Cast<AVehicleAIController>(FrontVehicle->GetController());
					
					if (FrontVehicleController)
					{
						// Öndeki aracın hızını al ve hedef hız olarak ayarla
						TargetSpeed = FrontVehicleController->CurrentSpeed;
					}
					else
					{
						// AI Controller yoksa, öndeki aracın hızını tahmin et veya dur
						TargetSpeed = FMath::Max(0.0f, CurrentSpeed * 0.8f); // Yavaşla
					}
				}
				else
				{
					// Güvenli mesafede, normal hıza devam et
					TargetSpeed = MaxSpeed;
				}
			}
			else
			{
				// Araç değil, normal engel - dur
				TargetSpeed = 0.0f;
			}
		}
	}
	else
	{
		// Engel yok veya rotamızda değil - maksimum hıza dön
		TargetSpeed = MaxSpeed;
		CurrentTrafficLightState = ETrafficLightState::Green; // Durumu güncelle
	}

	return bObstacleInPath;
}

float AVehicleAIController::SmoothSpeedTransition(float DeltaTime, float TransitionSpeed)
{
	// Linear Interpolation (Lerp) formülü:
	// NewSpeed = CurrentSpeed + (TargetSpeed - CurrentSpeed) * Alpha
	
	// Alpha değerini DeltaTime ve TransitionSpeed ile hesapla
	// Bu sayede hız değişimi frame rate'den bağımsız olur
	float Alpha = FMath::Clamp(TransitionSpeed * DeltaTime, 0.0f, 1.0f);
	
	// Lerp ile yeni hızı hesapla
	float NewSpeed = FMath::Lerp(CurrentSpeed, TargetSpeed, Alpha);
	
	// CurrentSpeed'i güncelle
	CurrentSpeed = NewSpeed;
	
	return NewSpeed;
}

float AVehicleAIController::UpdateSteering()
{
	// TargetSpline geçerli mi kontrol et. Geçerli değilse 0 döndür.
	if (!TargetSpline)
	{
		CurrentSteerValue = 0.0f;
		return 0.0f;
	}

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		CurrentSteerValue = 0.0f;
		return 0.0f;
	}

	const float SplineLength = TargetSpline->GetSplineLength();
	if (SplineLength <= 0.0f)
	{
		CurrentSteerValue = 0.0f;
		return 0.0f;
	}

	// Aracın (ControlledPawn) mevcut konumunu al
	FVector VehicleLocation = ControlledPawn->GetActorLocation();

	// Spline üzerinde araca en yakın mesafeyi bul (input key üzerinden mesafe hesaplanır)
	float ClosestInputKey = TargetSpline->FindInputKeyClosestToWorldLocation(VehicleLocation);
	float ClosestDistance = TargetSpline->GetDistanceAlongSplineAtSplineInputKey(ClosestInputKey);

	// Bu mesafeye LookAheadDistance ekleyerek hedef mesafeyi belirle
	float TargetDistance = ClosestDistance + LookAheadDistance;

	// Spline sonunu aşmayacak şekilde clamp et
	if (TargetDistance > SplineLength)
	{
		TargetDistance = SplineLength;
	}

	// Hedef mesafedeki dünya konumunu al (GetLocationAtDistanceAlongSpline = GetWorldLocationAtDistanceAlongSpline davranışı, World space)
	FVector TargetPoint = TargetSpline->GetLocationAtDistanceAlongSpline(TargetDistance, ESplineCoordinateSpace::World);

	// Şerit offset uygulaması: Spline üzerindeki hedef noktayı CurrentLaneOffset ile kaydır
	// Spline'ın o noktadaki sağ vektörünü al (spline'a dik yön)
	FVector SplineRightVector = TargetSpline->GetRightVectorAtDistanceAlongSpline(TargetDistance, ESplineCoordinateSpace::World);
	
	// Hedef noktayı spline'a dik yönde offset kadar kaydır
	// Pozitif offset = sağa, Negatif offset = sola
	TargetPoint = TargetPoint + (SplineRightVector * CurrentLaneOffset);

	// Direksiyon matematiği:
	// Araçtan hedef noktaya giden yön vektörünü hesapla (TargetDirection)
	FVector TargetDirection = (TargetPoint - VehicleLocation).GetSafeNormal();

	// Bu vektör ile aracın sağ yön vektörü (RightVector) arasında DotProduct yap
	FVector RightVector = ControlledPawn->GetActorRightVector();
	float SteerValue = FVector::DotProduct(TargetDirection, RightVector);

	// Çıkan sonucu CurrentSteerValue değişkenine ata ve döndür
	CurrentSteerValue = FMath::Clamp(SteerValue, -1.0f, 1.0f);

	return CurrentSteerValue;
}

bool AVehicleAIController::IsSidePathClear(bool bCheckRight)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// Aracın konumu ve yön vektörleri
	FVector VehicleLocation = ControlledPawn->GetActorLocation();
	FVector ForwardVector = ControlledPawn->GetActorForwardVector();
	FVector RightVector = ControlledPawn->GetActorRightVector();

	// Kontrol edilecek yön: sağ veya sol
	FVector SideDirection = bCheckRight ? RightVector : -RightVector;

	// Yan sensör mesafesi (şerit genişliği kadar)
	float SideSensorDistance = 300.0f; // Şerit genişliği (birim)

	// Yan sensör başlangıç ve bitiş noktaları
	FVector SideStartLocation = VehicleLocation + (ForwardVector * 100.0f); // Biraz önde başla
	FVector SideEndLocation = SideStartLocation + (SideDirection * SideSensorDistance);

	// LineTrace parametreleri
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(ControlledPawn); // Kendi aracımızı ignore et
	QueryParams.bTraceComplex = false;
	QueryParams.bReturnPhysicalMaterial = false;

	// Yan tarafta engel var mı kontrol et
	FHitResult SideHitResult;
	bool bHit = World->LineTraceSingleByChannel(
		SideHitResult,
		SideStartLocation,
		SideEndLocation,
		ECC_Visibility,
		QueryParams
	);

	// Eğer çarpışma varsa yol kapalı demektir
	return !bHit;
}

void AVehicleAIController::PlayHorn()
{
	// Korna sesini çal
	// Şimdilik sadece tanım olarak bırakılmıştır
	// UAudioComponent implementasyonu eklendiğinde burada ses çalınacak
	
	if (HornAudioComponent)
	{
		// Audio component varsa ses çal
		// HornAudioComponent->Play();
	}
	
	// Debug mesajı (isteğe bağlı)
	// UE_LOG(LogTemp, Log, TEXT("Horn played!"));
}

void AVehicleAIController::OnWeaponFireDetected()
{
	// Silah ateşi algılandı - panik modunu aktif et
	bIsPanicking = true;
	
	// Mevcut timer'ı iptal et (eğer varsa)
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(PanicTimerHandle);
		
		// 10 saniye sonra panik modunu kapat
		World->GetTimerManager().SetTimer(
			PanicTimerHandle,
			this,
			&AVehicleAIController::DisablePanicMode,
			10.0f,
			false // Tekrar eden değil, bir kez çalışacak
		);
	}
}

void AVehicleAIController::DisablePanicMode()
{
	// Panik modunu kapat
	bIsPanicking = false;
}
