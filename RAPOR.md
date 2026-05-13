BİLGİSAYAR MÜHENDİSLİĞİ
SİSTEM PROGRAMLAMA 2025-2026 BAHAR DÖNEMİ PROJESİ
tarsau Arşivleme Programı Proje Raporu
Mehmet Ali Tüfekçi b221210383 
Osman Oğuzhan Kırık b231210372
Ders: Sistem Programlama
Proje Adı: tarsau
Teslim Tarihi: 24 Mayıs 2026
GitHub Deposu: https://github.com/MAliTfkc/sistemprogramla_odev.git

!!NOT!! Bu ödev Hüseyin hoca ve Abdullah hoca ile konuşulup izin alınarak yapılmıştır. Gruptaki kişiler farklı hocalardan dersi almaktadır.

1. Giriş
Bu projede, tar, rar veya zip benzeri çalışan ancak sıkıştırma yapmayan tarsau adlı bir arşivleme programı geliştirilmiştir. Program Linux ortamında C dili ile yazılmış, komut satırından çalıştırılmak üzere tasarlanmış ve make ile derlenebilir hale getirilmiştir.

Projenin temel amacı, yalnızca ASCII metin dosyalarını özel .sau formatında birleştirmek ve bu arşiv dosyasını daha sonra aynı içerik ve dosya izinleriyle geri çıkarmaktır.

2. Proje Gereksinimleri

2.1. Arşiv oluşturma (-b)

Giriş dosyaları yalnızca ASCII metin dosyası olabilir.
Program yalnızca metin dosyalarını birleştirip tek bir arşiv dosyası üretir.
Çıktı dosyası -o parametresi ile belirtilir; belirtilmezse varsayılan ad a.sau kullanılır.
En fazla 32 giriş dosyası alınabilir.
Giriş dosyalarının toplam boyutu 200 MB'ı geçemez.
Uyumsuz dosya verildiğinde dosyaadi giriş dosyasının formatı uyumsuzdur! mesajı yazılır ve program kontrollü biçimde sonlanır.

2.2. Arşiv açma (-a)

-a parametresinden sonra en fazla 2 parametre alınır.
İlk parametre .sau uzantılı arşiv dosyasıdır.
İkinci parametre isteğe bağlı hedef dizindir; verilmezse dosyalar geçerli dizine çıkarılır.
Hedef dizin adında boşluk yoksa ve dizin mevcut değilse önce dizin oluşturulur.
Uygun olmayan arşiv dosyasında Arşiv dosyası uygunsuz veya bozuk! mesajı yazılır.
Açılan dosyalar, arşivlenirken kaydedilen okuma, yazma ve çalıştırma izinlerine geri döndürülür.

2.3. .sau dosya formatı

Arşiv dosyası iki bölümden oluşur:

Organizasyon bölümü

İlk 10 bayt, organizasyon bölümünün ASCII sayısal boyutunu içerir.
Her kayıt |dosya adı,izinler,boyut| biçimindedir.
Arşivlenmiş dosyalar
Dosya içerikleri ayırıcı kullanılmadan arka arkaya yazılır.
Her dosyanın boyutu organizasyon kaydından okunarak ayrıştırılır.
3. Geliştirme Süreci
3.1. Analiz ve tasarım
Öncelikle proje gereksinimleri incelenmiş, komut satırı kullanımı iki moda ayrılmıştır:

tarsau -b [giriş dosyaları] [-o cikti.sau]
tarsau -a arsiv.sau [hedef_dizin]
Ardından .sau formatı için veri yapısı ve dosya akışı belirlenmiştir. Organizasyon bölümü bellekte oluşturulduktan sonra 10 baytlık boyut alanı ve dosya içerikleri sırayla diske yazılacak şekilde tasarlanmıştır.

3.2. Modüler yapı
Kod okunabilirliği ve bakım kolaylığı için proje aşağıdaki dosyalara bölünmüştür:

Dosya	Görev
main.c	Komut satırı ayrıştırma ve -b / -a yönlendirmesi
archive.c	Arşiv oluşturma, açma, indeks üretme ve ayrıştırma
util.c	ASCII metin dosyası doğrulama
tarsau.h	Ortak sabitler, veri yapıları ve fonksiyon bildirimleri
Makefile	Derleme ve temizleme işlemleri
3.3. Uygulama adımları
Komut satırı argümanları ayrıştırıldı.
-b modunda giriş dosyaları için ASCII kontrolü, dosya sayısı ve toplam boyut sınırı uygulandı.
Her dosya için stat() ile boyut ve izin bilgisi alındı.
Organizasyon kayıtları oluşturulup arşiv dosyası yazıldı.
-a modunda .sau uzantısı, 10 baytlık boyut alanı ve indeks kayıtları doğrulandı.
Dosyalar hedef dizine çıkarıldı ve chmod() ile izinler geri yüklendi.
make ile derleme altyapısı hazırlandı.
3.4. Test süreci
Geliştirme sonrasında WSL Ubuntu ortamında aşağıdaki senaryolar denenmiştir:

İki metin dosyasından arşiv oluşturma
Arşivi hedef dizine açma
ASCII dışı bayt içeren dosya ile hata mesajı üretme
Var olmayan arşiv dosyası ile hata mesajı üretme
Tüm testler beklenen çıktıları vermiştir.

4. Program Mimarisi
4.1. Ortak tanımlar
define MAX_INPUT_FILES 32
define MAX_TOTAL_BYTES (200UL * 1024UL * 1024UL)
define INDEX_SIZE_FIELD 10
define DEFAULT_ARCHIVE "a.sau"
typedef struct {
char *name;
char *permissions;
size_t size;
} ArchiveEntry;

Bu yapı, arşivdeki her dosyanın adını, izinlerini ve boyutunu tutar.

4.2. Komut satırı yönetimi
main.c dosyasında program iki alt komuta yönlendirilir:

if (strcmp(argv[1], "-b") == 0) {
return run_bundle(argc, argv);
}
if (strcmp(argv[1], "-a") == 0) {
return run_extract(argc, argv);
}

-b modunda çıktı dosyası -o ile alınır; verilmezse a.sau kullanılır. -a modunda arşiv dosyası zorunlu, hedef dizin isteğe bağlıdır.

4.3. ASCII metin doğrulama
Giriş dosyalarının yalnızca ASCII olması util.c içinde kontrol edilir:

while ((read_count = fread(buffer, 1, sizeof(buffer), file)) > 0) {
for (index = 0; index < read_count; ++index) {
if (!is_ascii_byte(buffer[index])) {
fclose(file);
return 0;
}
}
}

Dosyadaki her bayt 0x7F değerini geçemez. Böylece ikili içerikli dosyalar arşive alınmaz.

4.4. Arşiv oluşturma
bundle_files() fonksiyonu şu adımları izler:

Her giriş dosyası için metin formatı ve düzenli dosya kontrolü yapılır.
stat() ile dosya boyutu ve izinleri okunur.
Toplam boyut 200 MB sınırı ile karşılaştırılır.
|dosya,izin,boyut| kayıtlarından oluşan organizasyon bölümü üretilir.
İlk 10 bayta organizasyon boyutu yazılır.
Dosya içerikleri sırayla arşive eklenir.
İzinler sekizlik formatta saklanır. Örneğin 644 veya 755.

4.5. Arşiv açma
extract_archive() fonksiyonu şu işlemleri yapar:

Arşiv adının .sau ile bitip bitmediğini kontrol eder.
İlk 10 bayttan organizasyon boyutunu okur.
Organizasyon bölümünü ayrıştırarak dosya listesini çıkarır.
Hedef dizin verilmiş ve adında boşluk yoksa dizini oluşturur.
Her dosyayı kayıttaki boyut kadar okuyup yazar.
chmod() ile izinleri geri yükler.
5. Derleme ve Çalıştırma
5.1. Derleme
Proje dizininde aşağıdaki komutlar kullanılır:

cd ~/b221210383
make

Temiz derleme için:

make clean
make

5.2. Kullanım örnekleri
Arşiv oluşturma:

./tarsau -b t1.txt t2.txt -o test.sau

Arşiv açma:

./tarsau -a test.sau cikti

Varsayılan çıktı adı ile arşiv oluşturma:

./tarsau -b t1.txt t2.txt

Bu durumda a.sau dosyası üretilir.

6. Test Senaryoları ve Ekran Çıktıları
6.1. Başarılı arşiv oluşturma ve açma
ali@Ali:/b221210383$ echo "ilk dosya" > t1.txt
ali@Ali:/b221210383$ echo "ikinci dosya" > t2.txt
ali@Ali:/b221210383$ ./tarsau -b t1.txt t2.txt -o test.sau
ali@Ali:/b221210383$ ./tarsau -a test.sau cikti
ali@Ali:/b221210383$ ls cikti
t1.txt t2.txt
ali@Ali:/b221210383$ cat cikti/t1.txt
ilk dosya
ali@Ali:~/b221210383$ cat cikti/t2.txt
ikinci dosya

Bu testte iki metin dosyası başarıyla arşivlenmiş, cikti dizinine açılmış ve içeriklerin değişmediği doğrulanmıştır.

6.2. Uyumsuz giriş dosyası testi
ali@Ali:/b221210383$ printf '\xff' > bozuk.bin
ali@Ali:/b221210383$ ./tarsau -b bozuk.bin -o x.sau
bozuk.bin giriş dosyasının formatı uyumsuzdur!

ASCII dışı bayt içeren dosya arşive alınmamış, ödevde istenen hata mesajı üretilmiştir.

6.3. Uygun olmayan arşiv dosyası testi
ali@Ali:~/b221210383$ ./tarsau -a olmayan.sau
Arşiv dosyası uygunsuz veya bozuk!

Var olmayan veya geçersiz arşiv dosyasında program çökmeden kontrollü hata mesajı vermiştir.

6.4. Derleme çıktısı
ali@Ali:/b221210383$ make
gcc -Wall -Wextra -std=c11 -O2 -c main.c
gcc -Wall -Wextra -std=c11 -O2 -c archive.c
gcc -Wall -Wextra -std=c11 -O2 -c util.c
gcc -Wall -Wextra -std=c11 -O2 -o tarsau main.o archive.o util.o
ali@Ali:/b221210383$ ls
Makefile archive.c archive.o main.c main.o tarsau tarsau.h util.c util.o

7. Karşılaşılan Sorunlar ve Çözümler

7.1. strdup ve mode_t derleme hataları
İlk derlemede strdup için örtük bildirim ve mode_t tipi ile ilgili hatalar alınmıştır. Çözüm olarak:

tarsau.h içine _POSIX_C_SOURCE 200809L tanımı eklendi.
archive.c dosyasına sys/types.h başlığı dahil edildi.
Bu değişikliklerden sonra proje sorunsuz derlenmiştir.

7.2. Dosya erişimi ve test ortamı
Geliştirme ve testler WSL Ubuntu ortamında yapılmıştır. Bu ortam, projenin Linux üzerinde çalışma şartı ile uyumludur.

8. GitHub ve Sürüm Takibi
Proje geliştirme süreci GitHub üzerinden yürütülmüştür. Depo adresi:

https://github.com/kullaniciadi/tarsau

Depoda kaynak kodlar, Makefile ve proje raporu bulunmaktadır. Geliştirme sırasında temel işlevler tamamlandıkça commit'ler ile ilerleme kayıt altına alınmıştır.

9. Sonuç
Bu projede sıkıştırma yapmayan, yalnızca ASCII metin dosyalarını birleştiren ve özel .sau formatında saklayan tarsau programı geliştirilmiştir. Program arşiv oluşturma ve arşiv açma işlemlerini komut satırından gerçekleştirmekte, hatalı giriş ve bozuk arşiv durumlarında ödevde belirtilen mesajları üretmektedir.

Yapılan testler, dosya içeriklerinin ve izin bilgilerinin arşivleme ve geri çıkarma sırasında korunabildiğini göstermiştir. Proje make ile derlenebilir durumdadır ve teslim için kaynak kod, rapor ve GitHub bağlantısı birlikte paketlenebilir.

10. Teslim Paketi
Sisteme yükleme için aşağıdaki dosyalar tek klasörde toplanmalıdır:

main.c
archive.c
util.c
tarsau.h
Makefile
RAPOR

Tarih: 13 Mayıs 2026