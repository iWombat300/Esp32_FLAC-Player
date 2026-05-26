# ESP32 FLAC Audio Player

Ein Arduino-Projekt zum Abspielen von FLAC-Audiodateien auf einem ESP32 von einer SD-Karte mit einem PCM5102A I2S DAC.

## Funktionen

- Wiedergabe von verlustfreien FLAC-Dateien.
- Steuerung über Taster für:
    - Play/Pause
    - Lautstärke erhöhen
    - Lautstärke verringern
    - Nächstes Lied
    - **Shuffle-Modus an/aus**
    - Nächste Playlist
- Playlist-System basierend auf einfachen `.txt`-Dateien.
- **Audio-Ansage des Playlist-Namens** beim Wechseln der Playlist.

## Benötigte Hardware

- ESP32 Development Board (jedes Modell ohne PSRAM sollte funktionieren)
- MicroSD-Kartenleser-Modul (SPI)
- PCM5102A I2S DAC Modul
- **6x Taster**
- MicroSD-Karte (formatiert als FAT32)
- Steckplatine und Verbindungskabel

## Benötigte Software & Bibliotheken

1.  **Arduino IDE** oder **PlatformIO** (empfohlen mit VS Code).
2.  **ESP32 Board-Unterstützung**: Muss in Ihrer IDE installiert sein.
3.  **ESP8266Audio Bibliothek**: Trotz des Namens funktioniert diese Bibliothek hervorragend mit dem ESP32 und ist sehr speichereffizient.
    - Installieren Sie sie über den Arduino Bibliotheksverwalter: `Sketch -> Bibliothek einbinden -> Bibliotheken verwalten...` und suchen Sie nach "ESP8266Audio".

## Hardware-Verkabelung

Passen Sie die Pins im Code (`ESP32_FLAC_Player.ino`) an, falls Sie eine andere Verkabelung verwenden.

### 1. SD-Kartenleser (SPI)

| SD-Modul | ESP32 Pin |
| :------- | :-------- |
| CS       | GPIO 5    |
| MOSI     | GPIO 23   |
| MISO     | GPIO 19   |
| SCK      | GPIO 18   |
| VCC      | 3.3V      |
| GND      | GND       |

### 2. I2S DAC (PCM5102A)

| DAC-Modul | ESP32 Pin |
| :-------- | :-------- |
| BCLK      | GPIO 25   |
| LRC / WS  | GPIO 26   |
| DOUT      | GPIO 22   |
| VCC       | 3.3V / 5V |
| GND       | GND       |

### 3. Taster

Jeder Taster wird zwischen dem entsprechenden GPIO-Pin und `GND` angeschlossen. Der interne Pull-Up-Widerstand des ESP32 wird verwendet, sodass keine externen Widerstände benötigt werden.

**Wichtiger Hinweis:** Die ursprünglichen Pins für "Leiser" (GPIO 35) und "Nächstes Lied" (GPIO 34) wurden geändert, da diese Pins keine internen Pull-Up-Widerstände haben. Bitte verwenden Sie die untenstehende, aktualisierte Pinbelegung.

| Funktion        | ESP32 Pin |
| :-------------- | :-------- |
| Play / Pause    | GPIO 32   |
| Lauter          | GPIO 33   |
| Leiser          | GPIO 13   |
| Nächstes Lied   | GPIO 12   |
| **Shuffle an/aus** | **GPIO 14**   |
| Nächste Playlist| GPIO 27   |

## Einrichtung der SD-Karte

1.  Formatieren Sie Ihre MicroSD-Karte mit dem **FAT32**-Dateisystem.
2.  Kopieren Sie Ihre `.flac`-Audiodateien in das Hauptverzeichnis (Root) der SD-Karte.
3.  Erstellen Sie eine oder mehrere Playlist-Dateien im Hauptverzeichnis.
    - Dies müssen einfache Textdateien mit der Endung `.txt` sein (z.B. `Rock.txt`, `Klassik.txt`).
    - In jeder Zeile einer Playlist-Datei steht der **exakte** Dateiname eines Liedes, das sich auf der Karte befindet.
4.  **(Optional) Erstellen Sie Ansage-Dateien für Ihre Playlists.**
    - Damit der Name einer Playlist beim Wechseln angesagt wird, erstellen Sie eine spezielle FLAC-Datei.
    - Der Name dieser Datei muss dem Namen der Playlist-Datei entsprechen, jedoch mit `_name.flac` anstelle von `.txt`.
    - **Beispiel:** Für die Playlist `Rock.txt` muss die Ansage-Datei `Rock_name.flac` heißen.

### Beispiel für eine Playlist-Datei (`Meine_Musik.txt`)

```
Lied_A.flac
Ein_anderes_Lied.flac
Track03.flac
```
Für diese Playlist würde das System beim Laden nach der Datei `Meine_Musik_name.flac` suchen und sie abspielen.

## Kompilieren und Hochladen

1.  Öffnen Sie die `ESP32_FLAC_Player.ino` in der Arduino IDE.
2.  Wählen Sie das richtige ESP32-Board aus (z.B. "ESP32 Dev Module").
3.  Wählen Sie den korrekten COM-Port aus.
4.  Klicken Sie auf "Hochladen".
5.  Öffnen Sie den Seriellen Monitor mit einer Baudrate von `115200`, um Debug-Informationen zu sehen.
