# Smokin' Guns — build iOS

Client iOS avec contrôles tactiles et multijoueur LAN.

## Prérequis

- macOS avec Xcode et le SDK iPhoneOS
- Dossiers `baseq3/`, `smokinguns/` et `ui/` à la racine du dépôt
- Compte Apple Developer pour installer sur appareil

## Ouvrir dans Xcode

```bash
open misc/ios/SmokinGuns.xcodeproj
```

Dans **Signing & Capabilities**, choisissez votre équipe de développement, puis sélectionnez un iPhone/iPad connecté et lancez (⌘R).

Le projet exécute `misc/ios/build-ios.sh` (compilation via Make + packaging `.app`).

## Build en ligne de commande

```bash
chmod +x misc/ios/*.sh
./misc/ios/setup_sdl2.sh   # une fois, si libSDL2-ios.a est absent
./misc/ios/build-ios.sh
```

Produit : `build/ios/SmokinGuns.app`

## Réseau LAN

- `NSLocalNetworkUsageDescription` est dans `Info.plist`
- Host : menu **Create Server** (listen server)
- Les autres clients se connectent à l’IP LAN de l’hôte

## Manettes et aide à la visée

Les manettes utilisent le traitement natif GameController pour éviter les conflits
avec SDL. Le stick gauche respecte le sens avant/arrière et le bouton PAD reste
accessible lorsqu’un clavier et une souris sont connectés.

Activez **Aim Assist** dans **Touch Controls** ou **Gamepad > Settings**. Les deux
interrupteurs partagent la même option sauvegardée, désactivée par défaut :
`seta in_aimAssist 1` (ou `0` pour désactiver).

La visée tactile et le stick droit ralentissent progressivement près d’un
adversaire visible, dans un cône de six degrés et à moins de 2048 unités.
L’aide ignore les coéquipiers, les morts, les spectateurs et les cibles masquées
par les murs ou portes fermées. Elle ne vise ni ne tire automatiquement.
La souris physique et la navigation des menus conservent leur sensibilité.

Tests de comportement sur le Mac : `sh misc/ios/tests/run-aim-assist.sh`.
