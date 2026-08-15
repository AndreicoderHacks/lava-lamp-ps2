# lavalamp-ps2

Simulare de lampă de lavă pentru PS2 homebrew (ps2sdk + gsKit).

## Cum funcționează
Bulele nu sunt poligoane — sunt un câmp de metaballs calculat pe CPU
(EE) la rezoluție mică (160x120), încărcat ca textură GS și desenat
pe tot ecranul cu filtrare bilinear. La fiecare pixel din câmp se
însumează `raza² / distanță²` pentru fiecare bulă vie; unde suma trece
de un prag ești "în interiorul" lichidului, iar lângă prag se face un
blend spre culoarea de glow — asta dă marginile moi și senzația că
bulele se contopesc organic, fără cost de geometrie pe GS.

Fizica (`physics.c`) e simplă și pe fixed-point (16.16), gândită să
arate a lampă de lavă reală:
- bulele se încălzesc aproape de bază și se răcesc spre vârf
- flotabilitatea depinde de temperatură (calde urcă, reci coboară)
- un wobble sinusoidal ușor le face să nu urce în linie dreaptă
- bulele apropiate și similare ca temperatură se pot contopi ocazional,
  iar slotul eliberat reapare de la bază

## Controale
- **D-pad sus/jos**: navighezi rândurile din meniul de setări
- **D-pad stânga/dreapta**: schimbi valoarea rândului selectat
- **START**: deschide/închide meniul de setări

Setări disponibile: culoare (5 palete), nr. de bule, căldură (viteză
de ridicare), intensitate lumină de bază, glow de fundal on/off.

## Structură
```
src/lavalamp.h   structuri, constante, fixed point
src/physics.c    simularea bulelor
src/metaball.c   calculul câmpului -> buffer RGBA
src/menu.c       paleta de culori + meniul de setări
src/input.c      citire pad (libpad), edge-detection pe butoane
src/main.c       init gsKit/DMA, textura, bucla principală
```

## Build (la fel ca la Liero PS2: ps2dev Docker + GitHub Actions)
Local, cu Docker:
```
docker build -t lavalamp-ps2 .
docker run --rm -v $(pwd):/out lavalamp-ps2
```
Va genera `lavalamp.elf` în folderul curent.

Sau push pe `main` / deschide un PR — workflow-ul din
`.github/workflows/build.yml` compilează automat și pune ELF-ul ca
artifact descărcabil.

## Testare pe PS2 real
La fel ca la Liero: copiază `lavalamp.elf` pe un stick USB și
lansează-l din uLaunchELF pe PS2-ul modat.

## Notă despre eroarea "gsKit.h: No such file or directory"
Dacă ai luat eroarea asta la primul build: cauza e că `gsKit` nu face
parte din ps2sdk propriu-zis, ci e un proiect separat instalat la
`$GSKIT` (implicit `/usr/local/ps2dev/gsKit` în imaginea
`ps2dev/ps2dev`, deja exportat ca variabilă de mediu acolo). Makefile-ul
de aici include acum `$(GSKIT)/include` + subdirectoarele lui
(`ee/dma/include`, `ee/gs/include`, `ee/toolkit/include`) și linkează
`-lgskit_toolkit -lgskit -ldmakit` din `$(GSKIT)/lib`. Dacă totuși dă
eroare la `GSFONTM` (tipul folosit pentru fontul built-in FONTM), verifică
numele exact din `$(GSKIT)/ee/toolkit/include/gsFontM.h` din imaginea ta
— unele versiuni de gsKit au denumit tipul puțin diferit.

## De ajustat/extins mai departe
- `FIELD_W/FIELD_H` din `lavalamp.h`: mărește dacă EE-ul duce ușor
  sarcina (mai multe bule = mai mult cost per pixel), micșorează dacă
  simți frame drop cu multe bule active
- momentan textul de meniu folosește fontul de debug din gsKit — dacă
  vrei ceva mai "lampă retro", following pasul următor logic ar fi un
  bitmap font custom
- `gsGlobal->Mode` e pus pe PAL; schimbă în `GS_MODE_NTSC` dacă
  televizorul/consola ta e pe NTSC
