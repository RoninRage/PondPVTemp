# Photovoltaik Temperatursensor

## Projektbeschreibung:

Es soll die Temperatur meines Schwimmteiches in definierten Intervallen in eine Datenbank geschrieben werden. Um keine weiteren Stromkabel verlegen zu müssen wird das System durch eine PV Zelle versorgt und verwendet Deep Sleep states des ESP32.

## Systemübersicht:



```mermaid

graph LR
A[Temp Sensor] -->B(ESP32)
C[Battery Voltage] -->B
B-->D[WIFI]
D-->E>MQTT]
E-->F[NodeRed]
F-->G(InfluxDB)

```

## Hardware:

### Verwendete Komponenten:

ESP 32 Dev Board

Photovoltaik Zelle

TP4056 Ladecontroller

18650 LiOn Battery

18650 Battery Halter



### Fritzing Diagram:

![](./doc/FritzingDiagrampng.png)

## Software (ESP32):



## NodeRed Flow:



## NodeRed Visualisierung:



