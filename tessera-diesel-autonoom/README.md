# Tessera Dieselprijzen — autonome gegevensbron (beta 0.2.0)

Deze plugin toont de goedkoopste dieselprijzen in een compacte Tessera-tegel. **Geen Home Assistant-sensor of ANWB-integratie nodig.** De **Tessera Screen Manager** blijft wel nodig en draait volgens Tessera's ontwerp binnen Home Assistant.

## Gegevenspad

ANWB POI v3 API → ingebouwde Tessera `fetch` (iedere 60 minuten) → plugin-tegel.

De Tessera-app voert de HTTPS-aanroep uit; **het ESP32-scherm haalt de gegevens niet rechtstreeks via HTTP op**. Volgens de officiële Tessera-documentatie is dat het ondersteunde pluginmechanisme.

- Gebied: vaste bounding box rond Annen, waarna op het scherm de exacte straal van **15 km hemelsbreed** wordt toegepast.
- Brandstof: uitsluitend `DIESEL`; onbekende prijzen worden niet verzonnen.
- Weergave: tot 8 stations (afhankelijk van afmetingen tegel), prijs oplopend, station en plaats.
- Bij mislukte nieuwe fetch toont Tessera mogelijk de laatst ontvangen data als verouderd.

## Installeren

1. Kopieer deze complete pluginmap naar `/config/tessera-plugins/dieselprijzen/`.
2. Open Tessera → Plugins → lokale plugin `Dieselprijzen` → installeer en bouw je scherm opnieuw.
3. Voeg de tegel `Diesel price list` toe in Layout.

## Bekende beperkingen

- **Niet op een echt Tessera-scherm gecompileerd of getest.** Gebruik dit als ontwikkelversie.
- De ingebouwde Tessera-fetch beperkt JSON-antwoorden tot 64 KiB en de opgehaalde gegevens tot 48 stations. Als de ANWB-respons groter is of het station ontbreekt in de eerste 48 resultaten, zal het overzicht niet compleet zijn.
- Per station worden de eerste vier brandstofsoorten doorgegeven. Een dieselvermelding buiten die vier wordt niet getoond.
- De fetch-map ondersteunt geen geografische of geneste brandstoffilters; dat gebeurt in de C++-tegel.
- De plugin vereist Tessera's plugin-API 0.27. Dit is dus geen zelfstandige ESPHome-firmware.

API: https://api.anwb.nl/routing/points-of-interest/v3/all

Zie de officiële [Tessera fetch-documentatie](https://github.com/MaxGramser/tessera-plugins/blob/main/docs/FETCH.md).
