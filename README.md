# Dieselprijzen (NL) for Tessera

> Beta. Uses your existing Home Assistant diesel-price source; it does **not** call ANWB itself.

A compact, sorted list of tank station names and diesel prices (€/L) on a Tessera screen. Station names appear on the left and prices on the right. Rows are ordered cheapest first; how many fit depends on tile height.

## Installation

1. Copy this directory into `/config/tessera-plugins/dieselprijzen/` on your Home Assistant host.
2. In Tessera, open **Plugins**, add the local plugin to your screen and build firmware.
3. In **Layout**, add the plugin tile **Diesel price list** and choose your sensor (`sensor.diesel_annen_lijst`, for example).
4. Resize the tile to 3×3 or 4×4 for more stations.

## Data format

Bind the tile to a sensor whose **attributes** contain one of the arrays `stations`, `lijst`, `results` or `data`. Items must contain a station name (`name`, `naam`, `station_name`, `title`, `tankstation`, `station`) and price (`price`, `prijs`, `diesel`, `diesel_price`, `fuel_price`, `value`). An optional `city` or `plaats` is appended to its name. Example:

```yaml
stations:
  - name: "Station A"
    plaats: "Annen"
    price: 1.699
  - name: "Station B"
    plaats: "Gieten"
    price: 1.729
```

If the selected entity has only a numeric price state, the tile shows one row with the entity's friendly name. To show several stations, select a list sensor. An HA state whose attributes use other keys will need a small adapter or a corresponding update of this plugin.

## Notes

- Your Pyscript remains responsible for fetching/updating ANWB data.
- This release does not provide navigation or a map; it shows names and prices only.
- Not yet compiled or tested on a physical Tessera device; verify with the official Tessera check and preview tools before deployment.
