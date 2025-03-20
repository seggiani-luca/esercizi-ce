#!/bin/sh

set -e

PAGINA_APPELLI="https://calcolatori.iet.unipi.it/appelli2.php?tag=c2as_qemu-v8"
VERSIONE=08
WORKSPACE="."

NC='\033[0m'

RED='\033[0;31m'
BLUE='\033[0;34m'
GREEN='\033[0;32m'

if [ -z "$1" ]; then
  printf "scarica-esame list         mostra tutte le date disponibili per il download\n"
  printf "scarica-esame yyyy-mm-dd   scarica l'esame della data yyyy-mm-dd\n" 
  exit 1
fi

DATE_APPELLI=$(curl -s "$PAGINA_APPELLI" | grep -o "value='[0-9_-]*'" | sed "s/value='\([0-9_-]*\)'/\1/")

if [ "$1" = "list" ]; then
    printf "Date disponibili:\n"
    for date in $DATE_APPELLI; do
        if [ -d "$WORKSPACE/$date" ]; then
            printf "$GREEN${date%_*}: Esame già scaricato $NC\n"
        else
            printf "$BLUE${date%_*}: Esame disponibile per il download $NC\n"
        fi
    done

else
    DATE_LIST=$(printf "$DATE_APPELLI" | sed "s/_$VERSIONE//g")
    if printf "$DATE_LIST" | grep -q "^$1$"; then
        if [ -d "$WORKSPACE/${1}_${VERSIONE}" ]; then
            printf "$RED Esame già scaricato $NC\n"
            exit 1
        fi
        uri="https://calcolatori.iet.unipi.it/appelli_download.php?data0=${1}_${VERSIONE}"
        tmp=$(mktemp -d)

        wget $uri -O $tmp/esame.tar.gz
        tar -xzvf $tmp/esame.tar.gz -C $WORKSPACE
        printf "$GREEN Esame scaricato correttamente $NC\n"
    else
        printf "$RED Esame non disponibile $NC\n"
        exit 1
    fi
fi
