#Proje 00 - Yeniden Yapılandırma

Bu projenin amacı statrup platformu projesinde yapılanları tekrar etmek

Bu proje yapılırken yapayzeka kullanılarak proje önerisi alınıp ayapazeka kullanılmadan yapılmıştır.

## 1. Adım

Bu adımda USER_BUTTON' a her basıldığında sistem modu `IDLE -> ACTIVE -> ERROR_SIM -> IDLE` olarak sırayla değişmektedir.

### GPIO Haritası

| Fonksiyon | Pin | Açıklama |
|---|---|----
| USER_BUTTON | PA0 | Kart üzerindeki user buton |
| LD4_GREEN | PD12 | Kart üzerindeki yeşil LED |
| LD3_ORANGE | PD13 | Kart üzerindeki turuncu LED |
| LD5_RED | PD14 | Kart üzerindeki kırmızı LED |
| LD6_BLUE | PD15 | Kart üzerindeki mavi LED |

### LED Mod Açıklaması

| Mod | LED |
|---|---|
| IDLE | Yeşil |
| ACTIVE | Mavi |
| ERROR_SIM | Kırmızı |

### Test Sonuçları

Kart üzerindeki `USER_BUTTON` a her basıldığında LED'ler sırası ile `Yeşil -> Mavi -> Kırmızı` şeklinde yanmaktadır.
