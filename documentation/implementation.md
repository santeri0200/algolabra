## Yleistä
Opinto-ohjelma: Tietojenkäsittelytieteen kandidaatti (TKT)

Harjoitustyön ydin: Harjoitustyössä toteutetaan kaksi pakkausalgoritmia. Toinen näistä on yleinen pakkausalgoritmi ja toinen kuvanpakkausalgoritmi. Yleinen pakkausalgoritmi toimii perustana kuvanpakkausalgoritmin vertailulle.

Projektin yleinen dokumentaatio (määrittelydokumentin ulkopuolella) toteutetaan englanniksi.

## Perustiedot
Mitä ohjelmointikieltä käytät?
- C++

Kerro myös mitä muita kieliä hallitset siinä määrin, että pystyt tarvittaessa vertaisarvioimaan niillä tehtyjä projekteja.
- Rust, Zig, Python, Javascript/Typescript

Mitä algoritmeja ja tietorakenteita toteutat työssäsi?
- QOI (Quite Ok Image format) ja DEFLATE -algoritmit
  - QOI on yksinkertainen kuvanpakkausalgoritmi, joka pakkaa kuvan pikselitasolla
  - DEFLATE on yleinen kompressiotyökalu, joka voi käyttää LZ77 ja Huffman codingia
    - DEFLATE toteutetaan ohjaajan suosituksesta

Minkä ongelman ratkaiset?
- Kuvanpakkausongleman, tai tarkemmin QOI tulisis olla kuvanpakkaustehokkuudeltaan parempi kuin yleinen pakkausalgoritmi

Mitä syötteitä ohjelma saa ja miten niitä käytetään?
- Ohjelmalle syötetään kuva (demo käyttöön kuva on raaka pikselidataa)
  - Molemmat vertailtavat pakkausalgoritmit pakkaavat kuvan omalla formaatillaan
  - Pakatut kuvat puretaan takaisin pikselidataksi, jotta käyttäjä voi vertailla kuvia, nähdä kompressiosuhteen, sekä pakkaus-/purkunopeuden
    - Kuvien uudelleen näyttämisen ideana on todeta algoritmien olevat häviöttömiä

Tavoitteena olevat aika- ja tilavaativuudet (esim. O-analyysit)
- QOI:n aika- ja tilavaatimukset ovat `O(n)`
- DEFLATE kaikilla kompressiovaihtoehdoilla olisi `O(n)` molemmissa vaativuuksissa
  - Huomioitavaa kuitenkin se että vaihtoehtojen välillä oleva vakiotermi `c` vaihtelee

Lähteet, joita aiot käyttää.
- QOI
  - https://qoiformat.org/qoi-specification.pdf
  - https://phoboslab.org/log/2021/11/qoi-fast-lossless-image-compression
- DEFLATE
  - https://en.wikipedia.org/wiki/Deflate
  - https://www.rfc-editor.org/rfc/rfc1951
