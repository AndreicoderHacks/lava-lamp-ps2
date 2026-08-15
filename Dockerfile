FROM ps2dev/ps2dev:latest

RUN apk add --no-cache make

WORKDIR /src
COPY . .

RUN make clean && make

CMD ["sh", "-c", "cp lavalamp.elf /out/ 2>/dev/null || true"]
