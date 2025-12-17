# Client Containerization Plan (TTYD + Cloud Run)

This document outlines the plan to containerize the `qkmj` CLI client using `ttyd` (a web-based terminal) and deploy it to Google Cloud Run.

## 1. Architecture

We will wrap the text-based game client in a WebSocket-enabled web server (`ttyd`). This allows users to play the game directly in their web browser without installing any software.

**Flow:**
`User Browser` (HTTPS) -> `Cloud Run Load Balancer` -> `ttyd` (Container) -> `qkmj` (Process) -> `mjgps` (Server)

## 2. Pros & Cons

### Pros
*   **Zero Installation**: Users just click a link to play.
*   **Platform Independent**: Works on Windows, Mac, Linux, Mobile (virtually).
*   **Security**: The client software runs in your controlled environment. The user only sees the terminal output. No risk of client-side binary modification/hacking.
*   **Scalability**: Cloud Run scales the number of containers based on demand.

### Cons
*   **Session State**: Cloud Run is stateless. If the user refreshes the page or the container restarts, the `qkmj` client process terminates.
    *   *Mitigation*: Our refactored architecture (Mongo-backed sessions) allows users to immediately rejoin their table upon reconnecting.
*   **Cost**: Cloud Run charges for active CPU time. A game session keeps the container "active" for the duration (e.g., 30-60 mins). This is more expensive than standard stateless APIs.
*   **Latency**: Adds a slight overhead (WebSocket wrapping), but usually negligible for a turn-based game like Mahjong.

## 3. Deployment Strategy (Cloud Run)

### Configuration
*   **WebSockets**: Supported out-of-the-box by Cloud Run.
*   **Timeout**: Set request timeout to maximum (3600 seconds / 1 hour) to allow long game sessions.
*   **Session Affinity**: Not strictly required since `ttyd` uses a persistent WebSocket connection, but recommended if using multiple instances heavily.
*   **Scaling**: Set `min-instances=0` to save costs when no one is playing.

### Docker Image Design
*   **Base**: Alpine Linux (lightweight).
*   **Components**:
    *   `ttyd`: The web-to-shell bridge.
    *   `qkmj`: The compiled game client.
    *   `ncurses`: Required library for the UI.
*   **Entrypoint**: `ttyd` listening on `$PORT`, executing `qkmj` with arguments from environment variables.

## 4. Implementation Steps

### Step 1: Create `Dockerfile.client`
A multi-stage build:
1.  **Builder**: Compile `qkmjclient` (and `ttyd` if not available via package).
2.  **Runtime**: Minimal Alpine image with libraries + binaries.

### Step 2: Build & Test Locally
```bash
docker build -f Dockerfile.client -t qkmj-web .
docker run -p 8080:8080 -e SERVER_IP=host.docker.internal qkmj-web
# Access http://localhost:8080
```

### Step 3: Cloud Run Deployment
```bash
gcloud run deploy qkmj-web \
  --image gcr.io/PROJECT/qkmj-web \
  --allow-unauthenticated \
  --timeout=3600 \
  --set-env-vars SERVER_IP=your-mjgps-server-ip
```

## 5. Next Action
I will proceed to create `Dockerfile.client` based on this plan.

```