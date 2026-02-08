#include "TrafficLight.h"
#include "TimerManager.h"
#include "Engine/Engine.h"
#include "Components/SceneComponent.h"

// Sets default values
ATrafficLight::ATrafficLight()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	// Tick kullanmıyoruz, bu yüzden false yapıyoruz (performans için)
	PrimaryActorTick.bCanEverTick = false;

	// Root component oluştur (Scene Component)
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	// Trigger Box Component oluştur
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(RootComponent);
	TriggerBox->SetBoxExtent(FVector(500.0f, 500.0f, 300.0f)); // Varsayılan boyut (X, Y, Z)
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // Sadece sorgulama için (fizik yok)
	TriggerBox->SetCollisionObjectType(ECC_WorldDynamic); // Collision channel
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore); // Tüm channel'lara ignore
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // Pawn'lara overlap

	// Varsayılan değerler
	CurrentState = ETrafficLightState::Green;
	GreenDuration = 10.0f;  // 10 saniye yeşil
	YellowDuration = 3.0f;  // 3 saniye sarı
	RedDuration = 8.0f;     // 8 saniye kırmızı
}

// Called when the game starts or when spawned
void ATrafficLight::BeginPlay()
{
	Super::BeginPlay();
	
	// Oyun başladığında ilk ışık değişimini zamanla
	// İlk durum Green olduğu için, GreenDuration sonra Yellow'a geçecek
	GetWorldTimerManager().SetTimer(
		LightSwitchTimerHandle,
		this,
		&ATrafficLight::SwitchLight,
		GreenDuration,
		false // Tekrar eden değil, bir kez çalışacak
	);
}

// Tick fonksiyonu kullanılmıyor (timer sistemi kullanıldığı için gerekli değil)
// void ATrafficLight::Tick(float DeltaTime)
// {
// 	Super::Tick(DeltaTime);
// }

void ATrafficLight::SwitchLight()
{
	// Mevcut duruma göre bir sonraki duruma geç
	switch (CurrentState)
	{
	case ETrafficLightState::Green:
		// Yeşilden sarıya geç
		CurrentState = ETrafficLightState::Yellow;
		
		// YellowDuration sonra kırmızıya geçmek için timer ayarla
		GetWorldTimerManager().SetTimer(
			LightSwitchTimerHandle,
			this,
			&ATrafficLight::SwitchLight,
			YellowDuration,
			false
		);
		break;

	case ETrafficLightState::Yellow:
		// Sarıdan kırmızıya geç
		CurrentState = ETrafficLightState::Red;
		
		// RedDuration sonra yeşile geçmek için timer ayarla
		GetWorldTimerManager().SetTimer(
			LightSwitchTimerHandle,
			this,
			&ATrafficLight::SwitchLight,
			RedDuration,
			false
		);
		break;

	case ETrafficLightState::Red:
		// Kırmızıdan yeşile geç
		CurrentState = ETrafficLightState::Green;
		
		// GreenDuration sonra sarıya geçmek için timer ayarla
		GetWorldTimerManager().SetTimer(
			LightSwitchTimerHandle,
			this,
			&ATrafficLight::SwitchLight,
			GreenDuration,
			false
		);
		break;
	}

	// Debug mesajı (isteğe bağlı - geliştirme sırasında kullanılabilir)
	// UE_LOG(LogTemp, Warning, TEXT("Traffic Light switched to: %d"), (int32)CurrentState);
}

ETrafficLightState ATrafficLight::GetCurrentState() const
{
	return CurrentState;
}

void ATrafficLight::SetLightState(ETrafficLightState NewState)
{
	// Mevcut timer'ı iptal et
	GetWorldTimerManager().ClearTimer(LightSwitchTimerHandle);

	// Yeni durumu ayarla
	CurrentState = NewState;

	// Yeni duruma göre timer'ı ayarla
	float Duration = 0.0f;
	switch (CurrentState)
	{
	case ETrafficLightState::Green:
		Duration = GreenDuration;
		break;
	case ETrafficLightState::Yellow:
		Duration = YellowDuration;
		break;
	case ETrafficLightState::Red:
		Duration = RedDuration;
		break;
	}

	// Timer'ı yeniden başlat
	if (Duration > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			LightSwitchTimerHandle,
			this,
			&ATrafficLight::SwitchLight,
			Duration,
			false
		);
	}
}
