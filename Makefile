.PHONY: build run-server run-client docker-build docker-up docker-down clean

build:
	mkdir -p build && cd build && cmake .. && make -j$(nproc)

run-server:
	./bin/server config/server.conf

run-client:
	./bin/client_gui

docker-build:
	docker build -t security-audit-server -f Dockerfile.server .

docker-up:
	docker-compose up -d

docker-down:
	docker-compose down

clean:
	rm -rf build bin logs/*.log

k8s-deploy:
	kubectl apply -f k8s/

k8s-status:
	kubectl get pods -l app=security-audit-server
	kubectl get services
