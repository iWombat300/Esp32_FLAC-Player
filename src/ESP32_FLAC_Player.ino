/*
 * ESP32 FLAC Audio Player
 *
 * Dieses Projekt ermöglicht das Abspielen von FLAC-Audiodateien von einer SD-Karte
 * über einen I2S DAC (z.B. PCM5102A) mit einem ESP32.
 *
 * Funktionen:
 * - Wiedergabe von FLAC-Dateien
 * - Steuerung über Taster: Lauter, Leiser, Play/Pause, Shuffle, Nächstes Lied, Playlist wechseln
 * - Playlist-Management über .txt-Dateien auf der SD-Karte
 *
 * Benötigte Bibliotheken:
 * - "ESP8266Audio" von Earle F. Philhower, III (Funktioniert auch für ESP32)
 *   -> Installieren Sie diese über den Arduino Bibliotheksverwalter.
 *
 * Hardware-Verbindungen:
 *
 * 1. SD-Kartenleser (SPI):
 *    - ESP32 Pin GPIO 5  -> SD CS
 *    - ESP32 Pin GPIO 23 -> SD MOSI
 *    - ESP32 Pin GPIO 19 -> SD MISO
 *    - ESP32 Pin GPIO 18 -> SD SCK
 *
 * 2. I2S DAC (PCM5102A):
 *    - ESP32 Pin GPIO 25 -> I2S BCLK (Bit Clock)
 *    - ESP32 Pin GPIO 26 -> I2S LRC (Left/Right Clock) / WS (Word Select)
 *    - ESP32 Pin GPIO 22 -> I2S DOUT (Data Out)
 *    - VCC -> 3.3V oder 5V (je nach Modul)
 *    - GND -> GND
 *
 * 3. Taster:
 *    - Jeder Taster wird zwischen dem definierten GPIO-Pin und GND angeschlossen.
 *    - Der interne Pull-Up-Widerstand des ESP32 wird verwendet.
 *
 * SD-Karten-Struktur:
 * - Alle FLAC-Dateien (.flac) im Hauptverzeichnis.
 * - Playlist-Dateien (.txt) im Hauptverzeichnis.
 * - Eine Playlist-Datei ist eine einfache Textdatei, bei der jeder Zeile der exakte Dateiname eines Liedes steht (z.B. "song1.flac").
 *
 */

// Bibliotheken einbinden
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "AudioFileSourceSD.h"
#include "AudioGeneratorFLAC.h"
#include "AudioOutputI2S.h"

// === PIN-DEFINITIONEN ===
// Passen Sie diese Pins an Ihre Verkabelung an

// I2S Pins
#define I2S_BCLK 25
#define I2S_LRC  26
#define I2S_DOUT 22

// SD-Karten Pins (Standard VSPI)
#define SD_CS    5
#define SD_MOSI  23
#define SD_MISO  19
#define SD_SCK   18

// Taster-Pins
// HINWEIS: GPIOs 34 und 35 haben keine internen Pull-Up-Widerstände!
// Daher wurden die Pins für "Leiser" und "Nächstes Lied" auf GPIOs geändert,
// die interne Pull-Ups unterstützen.
#define PLAY_PAUSE_PIN  32
#define VOL_UP_PIN      33
#define VOL_DOWN_PIN    13 // Geändert von 35
#define NEXT_SONG_PIN   12 // Geändert von 34
#define SHUFFLE_PIN     14 // Taster zum Aktivieren/Deaktivieren von Shuffle
#define NEXT_PLAYLIST_PIN 27

// === GLOBALE VARIABLEN ===
AudioGeneratorFLAC *flac;
AudioFileSourceSD *file;
AudioOutputI2S *out;

// Playlist-Verwaltung
const int MAX_PLAYLISTS = 10;
const int MAX_SONGS_PER_PLAYLIST = 100;
String playlists[MAX_PLAYLISTS];
String songs[MAX_SONGS_PER_PLAYLIST];
int playlistCount = 0;
int songCount = 0;
int currentPlaylistIndex = 0;
int currentSongIndex = 0;

// Zustandsvariablen
bool isPlaying = false;
bool shuffleActive = false;
float currentGain = 0.1; // Globale Variable für die Lautstärke

// Taster-Debouncing
long lastDebounceTime = 0;
long debounceDelay = 250; // 250ms Verzögerung

// === FUNKTIONSPROTOTYPEN ===
void scanPlaylists();
void loadPlaylist(int playlistIndex);
void playSong(int songIndex);
void playFileBlocking(const char* path);
void handleButtons();
void nextSong();
void stopPlaying();

void setup() {
  Serial.begin(115200);
  Serial.println("\nESP32 FLAC Player");
  
  // Seed für den Zufallsgenerator
  randomSeed(analogRead(0));

  // Taster initialisieren
  pinMode(PLAY_PAUSE_PIN, INPUT_PULLUP);
  pinMode(VOL_UP_PIN, INPUT_PULLUP);
  pinMode(VOL_DOWN_PIN, INPUT_PULLUP);
  pinMode(NEXT_SONG_PIN, INPUT_PULLUP);
  pinMode(SHUFFLE_PIN, INPUT_PULLUP);
  pinMode(NEXT_PLAYLIST_PIN, INPUT_PULLUP);

  // Audio-Ausgang initialisieren
  // Wir verwenden den Konstruktor mit expliziten Puffer-Einstellungen, um Stottern zu vermeiden.
  // Port 0, Mode 0 (EXTERNAL_I2S), 16 Puffer à 1024 Bytes.
  // Diese Werte sind stabil und verhindern den vorherigen Absturz.
  out = new AudioOutputI2S(0, 0, 16, 1024); 
  out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  out->SetGain(currentGain); // Start-Lautstärke setzen

  // SD-Karte initialisieren
  // Die Frequenz von 20MHz war zu hoch. Wir gehen zurück auf 8MHz als stabilen Kompromiss.
  if (!SD.begin(SD_CS, SPI, 8000000)) {
    Serial.println("Fehler beim Initialisieren der SD-Karte!");
    while (1); // Anhalten
  }
  Serial.println("SD-Karte initialisiert.");

  // Playlists suchen und erste Playlist laden
  scanPlaylists();
  if (playlistCount > 0) {
    loadPlaylist(currentPlaylistIndex);
  } else {
    Serial.println("Keine Playlists (.txt Dateien) auf der SD-Karte gefunden.");
  }
}

void loop() {
  // Taster abfragen
  handleButtons();

  // Audio-Loop
  if (flac && flac->isRunning()) {
    if (!flac->loop()) {
      // Lied ist zu Ende
      stopPlaying();
      // Wenn der Player im "Play"-Modus war, automatisch das nächste Lied starten
      if (isPlaying) {
         nextSong();
      }
    }
  }
}

// Sucht auf der SD-Karte nach .txt-Dateien
void scanPlaylists() {
  Serial.println("Suche nach Playlists...");
  File root = SD.open("/");
  playlistCount = 0;
  while (true) {
    File entry = root.openNextFile();
    if (!entry) {
      break; // Keine weiteren Dateien
    }
    if (String(entry.name()).endsWith(".txt")) {
      if (playlistCount < MAX_PLAYLISTS) {
        playlists[playlistCount] = entry.name();
        Serial.println("Gefunden: " + playlists[playlistCount]);
        playlistCount++;
      }
    }
    entry.close();
  }
  root.close();
}

// Lädt die Lied-Dateinamen aus einer Playlist-Datei
void loadPlaylist(int playlistIndex) {
  if (playlistIndex >= playlistCount) return;
  
  stopPlaying();
  songCount = 0;
  currentSongIndex = 0;
  
  String playlistName = playlists[playlistIndex];
  Serial.println("Lade Playlist: " + playlistName);

  File playlistFile = SD.open("/" + playlistName);
  if (playlistFile) {
    while (playlistFile.available()) {
      String songName = playlistFile.readStringUntil('\n');
      songName.trim(); // Entfernt Leerzeichen und Zeilenumbrüche
      if (songName.length() > 0 && songCount < MAX_SONGS_PER_PLAYLIST) {
        songs[songCount] = songName;
        Serial.println("  -> Lied hinzugefügt: " + songs[songCount]);
        songCount++;
      }
    }
    playlistFile.close();
  } else {
    Serial.println("Fehler beim Öffnen der Playlist-Datei.");
  }
}

// Spielt ein Lied aus der aktuellen Playlist
void playSong(int songIndex) {
  if (songIndex >= songCount) return;

  stopPlaying();

  String songName = songs[songIndex];
  String fullPath = "/" + songName;

  file = new AudioFileSourceSD(fullPath.c_str());
  flac = new AudioGeneratorFLAC();
  
  Serial.println("Spiele: " + songName);
  if (!flac->begin(file, out)) {
    Serial.println("Fehler beim Starten des FLAC-Decoders.");
    stopPlaying();
  } else {
    isPlaying = true;
  }
}

// Spielt eine einzelne Datei blockierend ab (z.B. für Ansagen)
void playFileBlocking(const char* path) {
  AudioFileSourceSD *tempFile = new AudioFileSourceSD(path);
  if (!tempFile->isOpen()) {
    Serial.println("Ansage-Datei nicht gefunden: " + String(path));
    delete tempFile;
    return;
  }
  
  AudioGeneratorFLAC *tempFlac = new AudioGeneratorFLAC();
  if (tempFlac->begin(tempFile, out)) {
    Serial.println("Spiele Ansage: " + String(path));
    while (tempFlac->loop()) {
      // Blockiert, bis die Datei zu Ende ist
    }
    tempFlac->stop();
  } else {
    Serial.println("Fehler beim Abspielen der Ansage.");
  }
  
  delete tempFlac;
  delete tempFile;
}


// Stoppt die aktuelle Wiedergabe und gibt Speicher frei
void stopPlaying() {
  if (flac) {
    flac->stop();
    delete flac;
    flac = NULL;
  }
  if (file) {
    file->close();
    delete file;
    file = NULL;
  }
  // isPlaying wird hier NICHT auf false gesetzt, damit die Autoplay-Funktion weiß,
  // ob sie das nächste Lied starten soll.
}

// Wählt das nächste Lied aus (mit optionaler Shuffle-Logik)
void nextSong() {
  if (songCount == 0) return;

  if (shuffleActive) {
    int nextIndex = random(songCount);
    // Verhindert, dass dasselbe Lied zweimal hintereinander gespielt wird (wenn möglich)
    if (songCount > 1 && nextIndex == currentSongIndex) {
      nextIndex = (nextIndex + 1) % songCount;
    }
    currentSongIndex = nextIndex;
  } else {
    currentSongIndex++;
    if (currentSongIndex >= songCount) {
      currentSongIndex = 0; // Wieder von vorne anfangen
    }
  }
  playSong(currentSongIndex);
}

// Verarbeitet die Taster-Eingaben
void handleButtons() {
  if ((millis() - lastDebounceTime) < debounceDelay) {
    return; // Debouncing
  }

  // Play / Pause
  if (digitalRead(PLAY_PAUSE_PIN) == LOW) {
    lastDebounceTime = millis();
    if (!flac || !flac->isRunning()) { // Wenn nichts läuft
      isPlaying = true; // Wiedergabemodus aktivieren
      playSong(currentSongIndex);
    } else { // Wenn etwas läuft
      isPlaying = false; // Wiedergabemodus stoppen
      stopPlaying();
      Serial.println("Wiedergabe gestoppt (Pause).");
    }
  }

  // Lauter
  else if (digitalRead(VOL_UP_PIN) == LOW) {
    lastDebounceTime = millis();
    currentGain += 0.05;
    if (currentGain > 1.0) currentGain = 1.0;
    out->SetGain(currentGain);
    Serial.println("Lautstärke: " + String(currentGain));
  }

  // Leiser
  else if (digitalRead(VOL_DOWN_PIN) == LOW) {
    lastDebounceTime = millis();
    currentGain -= 0.05;
    if (currentGain < 0.0) currentGain = 0.0;
    out->SetGain(currentGain);
    Serial.println("Lautstärke: " + String(currentGain));
  }

  // Nächstes Lied
  else if (digitalRead(NEXT_SONG_PIN) == LOW) {
    lastDebounceTime = millis();
    Serial.println("Nächstes Lied...");
    if (isPlaying) {
      nextSong();
    }
  }
  
  // Shuffle an/aus
  else if (digitalRead(SHUFFLE_PIN) == LOW) {
    lastDebounceTime = millis();
    shuffleActive = !shuffleActive;
    if (shuffleActive) {
      Serial.println("Shuffle aktiviert.");
    } else {
      Serial.println("Shuffle deaktiviert.");
    }
  }
  
  // Nächste Playlist
  else if (digitalRead(NEXT_PLAYLIST_PIN) == LOW) {
    lastDebounceTime = millis();
    if (playlistCount > 0) {
      stopPlaying();
      isPlaying = false;
      
      currentPlaylistIndex++;
      if (currentPlaylistIndex >= playlistCount) {
        currentPlaylistIndex = 0;
      }
      
      Serial.println("Wechsle zu nächster Playlist.");
      
      // Playlist-Namen als FLAC abspielen
      String playlistName = playlists[currentPlaylistIndex];
      String announcementName = playlistName;
      announcementName.replace(".txt", "_name.flac");
      playFileBlocking(("/" + announcementName).c_str());
      
      // Jetzt die eigentliche Playlist laden
      loadPlaylist(currentPlaylistIndex);
    }
  }
}