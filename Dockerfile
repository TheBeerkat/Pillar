FROM alpine:3.23 AS build
WORKDIR /app
RUN apk add build-base
COPY . .
RUN make compile

FROM alpine:3.23 AS final
WORKDIR /app
COPY --from=build /app/main .
CMD ["/app/main", "0.0.0.0", "8080"]
