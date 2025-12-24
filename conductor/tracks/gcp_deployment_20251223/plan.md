# Plan: GCP Cloud Deployment & CI/CD Integration

## Phase 1: Artifact Registry & GitHub Actions Setup
**Goal:** Establish the foundation for container image management and automated builds.

- [ ] Task: Create Google Artifact Registry via Terraform.
- [ ] Task: Configure GitHub Actions Secrets for GCP authentication.
- [ ] Task: Implement CI workflow to build and test C code.
- [ ] Task: Implement CD workflow to build and push Docker images to Artifact Registry.
- [ ] Task: Conductor - User Manual Verification 'Phase 1: Artifact Registry & GitHub Actions Setup' (Protocol in workflow.md)

## Phase 2: Server Deployment (GCE)
**Goal:** Provision and deploy the `mjgps` server to Google Compute Engine.

- [ ] Task: Finalize Terraform for GCE instance and networking (firewall).
- [ ] Task: Implement deployment script/automation to pull and run the server container on GCE.
- [ ] Task: Verify server connectivity and MongoDB integration on GCE.
- [ ] Task: Conductor - User Manual Verification 'Phase 2: Server Deployment (GCE)' (Protocol in workflow.md)

## Phase 3: Client Deployment (Cloud Run)
**Goal:** Deploy the `qkmj` client to Google Cloud Run.

- [ ] Task: Finalize Terraform for Cloud Run service.
- [ ] Task: Implement deployment automation for Cloud Run.
- [ ] Task: Verify client connectivity to the GCE server and AI backend.
- [ ] Task: Conductor - User Manual Verification 'Phase 3: Client Deployment (Cloud Run)' (Protocol in workflow.md)
