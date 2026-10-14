# Project-03A — STM32 Multi-Peripheral Learning Bridge

## 1. Proje Özeti

### 1.1 Amaç

Bu projenin amacı STM32F407 tabanlı bir sistemde birden fazla çevre birimini
düzenli, modüler ve hata kontrollü şekilde kullanmayı öğrenmektir.

İlk aşamada kart üzerindeki LIS3DSH accelerometer SPI üzerinden kontrol
edilecektir. Daha sonraki aşamalarda timer/input capture ile HC-SR04
entegrasyonu ve birden fazla peripheral'in aynı firmware içerisinde
non-blocking biçimde çalıştırılması hedeflenmektedir.

### 1.2 Bu Projede Geliştirilmek İstenen Yetkinlikler

- Datasheet okuma
- Kart şeması okuma
- SPI protokolünü anlayarak kullanma
- Register tabanlı peripheral erişimi
- Modüler driver geliştirme
- Error handling
- Debugging ve root-cause analizi
- Timer / input capture
- Interrupt ve volatile kullanımı
- Non-blocking firmware yaklaşımı
- Hardware–firmware problemlerini birbirinden ayırabilme
- Teknik kararları kaynaklarla gerekçelendirebilme

### 1.3 Proje Kapsamı

### 1.4 Proje Dışı Bırakılanlar


## 2. Kullanılan Donanım ve Yazılımlar

### 2.1 Donanım

### 2.2 Yazılım / Araçlar


## 3. Proje Aşamaları

### Adım 1 — Donanım ve SPI Cihazını Tanıma

Kart ve accelerometer modeli doğrulanacak. Kart şeması ve sensör datasheet'i
incelenecek. SPI bağlantıları ve ilk register erişimi için gereken bilgiler
çıkarılacak.

### Adım 2 — Minimum SPI / WHO_AM_I

SPI üzerinden yalnızca WHO_AM_I register'ı okunacak.

### Adım 3 — Accelerometer Driver

Register read/write ve X/Y/Z veri okuma fonksiyonları modüler hâle getirilecek.

### Adım 4 — SPI'yi Doğrulama ve Derinleştirme

SPI davranışı debugger ve gerektiğinde logic analyzer ile doğrulanacak.

### Adım 5 — Timer / Input Capture

HC-SR04 timer input capture yöntemiyle okunacak.

### Adım 6 — Multi-Peripheral Entegrasyonu

SPI + timer + UART aynı firmware içerisinde düzenli ve mümkün olduğunca
non-blocking şekilde çalıştırılacak.



## 4. Adım 1 — Donanım ve SPI Cihazını Tanıma

### 4.1 Amaç

Kart üzerindeki accelerometer modelini kesin olarak belirlemek ve
STM32 ile sensör arasındaki SPI haberleşmesini kod yazmadan önce
teknik kaynaklardan anlamak.

### 4.2 Başarı Kriterleri

Adım 1 tamamlandığında:

- Kart revizyonu biliniyor.
- Accelerometer modeli doğrulanmış.
- Kullanılan SPI peripheral belirlenmiş.
- SCK, MOSI, MISO ve CS pinleri belirlenmiş.
- SPI clock sınırı biliniyor.
- CPOL ve CPHA belirlenmiş.
- SPI mode belirlenmiş.
- WHO_AM_I register adresi bulunmuş.
- Beklenen WHO_AM_I değeri bilinmiş.
- SPI register read formatı anlaşılmış.

### 4.3 Kart ve Sensör Tanımlama

**Geliştirme kartı:** STM32F407G-DISC1  
**Ana kart referansı:** MB997E / MB997-F407VGT6-E01  
**MCU:** STM32F407VGT6  
**On-board motion sensor:** LIS3DSH  
**Haberleşme arayüzü:** SPI

### 4.4 STM32 ↔ Sensör Pin Bağlantıları

| İşlev | LIS3DSH | STM32 pini | STM32 Peripheral |
|---|---|---|---|
| SPI Clock | SPC | PA5 | SPI1_SCK |
| Master → Sensor | SDI | PA7 | SPI1_MOSI |
| Sensor → Master | SDO | PA6 | SPI1_MISO |
| Chip Select | CS | PE3 | GPIO |
| Interrupt 1 | INT1 | PE0 | GPIO/EXTI |
| Interrupt 2 | INT2 | PE1 | GPIO/EXTI |

### 4.5 SPI Gereksinimleri

| Özellik | Değer | 
| --- | --- |
| Maximum SPI clock | 10 MHz |
| Minimum Clock Period | 100 ns |
| Clock idle state | HIGH |
| Data change edge | Falling |
| Data Sanpling Edge | Rising |
| CPOL | 1 / HIGH |
| CPHA | 1 / Second Edge |
| SPI Mode | Mode 3 |
| CS active level | LOW | 

### 4.6 WHO_AM_I ve Register Yapısı

| Özellik | Değer |
|---|---|
| Register | WHO_AM_I |
| Address | 0x0F |
| Access | Read only |
| Expected value | 0x3F |

#### SPI Read Formatı

SPI transaction iki byte olarak ele alındı:

- Byte 1:
  - bit 7: R/W
  - bit 6:0: register address
- Byte 2:
  - write işleminde data
  - read işleminde master dummy byte gönderir, slave data döndürür

R/W = 1 → Read
R/W = 0 → Write

### 4.7 Teknik Kararlar ve Gerekçeleri

#### KARAR-001 — SPI kullanılacak

**Karar:**
On-board LIS3DSH ile SPI üzerinden haberleşilecek.

**Gerekçe:**
STM32F4DISCOVERY kartında motion sensor STM32 tarafından SPI üzerinden
kontrol edilmektedir.

**Kaynak:**
STMicroelectronics UM1472 — Motion Sensor bölümü.

**Uygulamaya etkisi:**
STM32 tarafında ilgili SPI peripheral yapılandırılacaktır.

#### KARAR-002 — SPI Mode X kullanılacak

**Karar:**
SPI Mode 3 kullanılacak.

**Gerekçe:**
LIS3DSH SPI clock hattı iletişim yokken HIGH durumundadır.
Veri falling edge'de değiştirilir ve rising edge'de örneklenir.
Bu davranış CPOL=1 ve CPHA=1'e, yani SPI Mode 3'e karşılık gelir.

**Kaynak:**
STMicroelectronics — LIS3DSH Datasheet, SPI bus interface.

**Doğrulama:**
Adım 2'de WHO_AM_I register okuması ile ilk işlevsel doğrulama yapılacak.

### 4.8 Öğrendiklerim

- Kart şeması/user manual üzerinden STM32 ile sensör arasındaki gerçek SPI pin bağlantılarını çıkardım.
- LIS3DSH datasheet'indeki timing bilgisinden CPOL, CPHA ve SPI Mode'u belirledim.
- SPI read command byte'ının R/W biti ve register adresinden nasıl oluşturulduğunu öğrendim.

### 4.9 Adım 1 Sonuç

Adım 1 tamamlandı. LIS3DSH modeli, STM32 bağlantıları, SPI Mode 3 gereksinimi,
WHO_AM_I register adresi/değeri ve SPI read formatı teknik kaynaklardan doğrulandı.

## 5. Adım 2 — Minimum SPI / WHO_AM_I

### 5.1 Adım 2 Amaç

Bu adımdaki maç en basit haliyle tek bir SPI haberleşmesi ile kullanılan sensörün doğru registerinden WHO_AM_I değerini okumak. Okunan değeri datasheet yardımıyla doğrulamak.

### 5.2 Başarı Kriterleri

- Tek bir SPI haberleşmesi ile doğru WHO_AM_I değerini okumak.
- Yapılan okumanın çalışıp çalışmamasına göre doğru şekikde OK/FAULT algoritmsının kurulması.
- En az 5 reset sonrası sistemin hala kararlı şekilde çalışmaya devam etmesi.

### 5.3 Uygulama

- SPI1 Mode 3 kullanıldı.
- SPI clock 6.25 MHz (/2 prescaler).
- CS pini PE3 GPIO olarak kontrol edildi.
- Read command + dummy byte ile 2 byte SPI transferi yapıldı.
- HAL_SPI_TransmitReceive sonucu kontrol edildi.

### 5.4 Krşılaşılan Problemler

- İlk SPI okumasında 0xFF 0xFF alındı.
- INFO1 ve INFO2 registerları da okunarak test genişletildi.
- Okuma sırası değiştirildiğinde hangi register ilk okunursa onun başarısız olduğu görüldü.
- SPI peripheral, ilk transferden önce CS HIGH durumundayken enable edildi.
- Bu değişiklikten sonra ilk transfer dahil tüm okumalar doğru çalıştı.

### 5.5 Sonuç

WHO_AM_I = 0x3F
INFO1    = 0x21
INFO2    = 0x00

6.25 MHz SPI hızında çoklu reset testlerinde sonuçlar kararlı şekilde tekrarlandı.
Adım 2 tamamlandı.

## 6. Test ve Doğrulama Kayıtları


## 7. Debug / Karşılaşılan Problemler


## 8. Bilinen Eksikler ve Sınırlamalar


## 9. Kullanılan Kaynaklar

### Donanım / Kart

1. STMicroelectronics — UM1472
   *Discovery kit with STM32F407VG MCU — User Manual*
   Kullanım:
   - Board revision doğrulama
   - Motion sensor bağlantıları
   - STM32 pin eşleşmeleri

### Sensör

2. STMicroelectronics — LIS3DSH Datasheet
   *MEMS digital output motion sensor — ultra-low-power high-performance
   three-axis accelerometer*
   DocID022405 Rev 3.

   Kullanım:
   - SPI electrical/timing özellikleri
   - SPI read/write protocol
   - Register map
   - WHO_AM_I
   - Daha sonra X/Y/Z register'ları


## 10. Sonraki Adım
