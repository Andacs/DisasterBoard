# Rehber karakterler (A = Mert, B = Asil, C = Yiğit)

Oyun bu klasörü **dosyadan** okur, Unreal'e import gerekmez. Dosya yoksa koddan çizilmiş yer tutucu figür görünür.

Her karakter için **iki poz**, ikisi de 320×512 şeffaf PNG (şablon: `Docs/Sablonlar/2D/Karakterler/`):

- `A_durus.png`, `B_durus.png`, `C_durus.png`: **duruş**. Kameraya bakıyor, iki kol aşağıda.
  Ayaklar resmin **alt kenarında**, figür yatayda ortada.
- `A_goster.png`, `B_goster.png`, `C_goster.png`: **gösterme**. Aynı karakter, aynı yerde ve boyda; ekranda sağdaki
  kolunu yukarı-sağa kaldırmış, eli şablondaki yeşil dairenin civarında (çerçevenin %90 sağı, %45 aşağısı).

Oyun, rehber bir şeyi gösterirken (tarayıcı, sipariş kartı, bina...) gösterme pozunu, diğer zamanlarda duruş pozunu
çizer; elden hedefe noktalı bir iz çıkar. Hedef soldaysa resim aynalanır. İki poz üst üste konunca gövde
kıpırdamamalı: aynı tuvalde, aynı yerde çiz.

Büyük tuvale çizildiyse (ör. 1600×1600) sorun değil: `python Tools/varlik_kontrol.py --karakter A <durus.png> <goster.png>`
ikisini aynı kesimle kırpıp 320×512'ye oturtur. **Şeffaflık şart:** PNG'yi mesaj uygulamasından "fotoğraf" olarak
gönderince arka plan siyaha döner; dosya / belge olarak gönder.
