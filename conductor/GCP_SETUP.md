# GCP and GitHub Actions Setup Guide

To enable automated deployment to Google Cloud Platform, you need to configure the following GitHub Secrets in your repository settings (**Settings > Secrets and variables > Actions**).

## Required Secrets

1.  **`GCP_PROJECT_ID`**: Your Google Cloud Project ID.
2.  **`GCP_SA_KEY`**: The JSON key of a Google Cloud Service Account with the following permissions:
    - `Artifact Registry Writer` (to push Docker images)
    - `Compute Admin` (to manage GCE instances if deploying via Actions)
    - `Cloud Run Admin` (to deploy the client to Cloud Run)
    - `Service Account User` (required to deploy Cloud Run services)

## Steps to create the Service Account and Key

1.  **Enable APIs**: Ensure the following APIs are enabled in your GCP project:
    - Artifact Registry API
    - Compute Engine API
    - Cloud Run API
    - Cloud Build API

2.  **Create Service Account**:
    ```bash
    gcloud iam service-accounts create github-actions-deploy \
      --display-name="GitHub Actions Deployment Service Account"
    ```

3.  **Assign Roles**:
    ```bash
    export PROJECT_ID=$(gcloud config get-value project) \
    
    # Artifact Registry
    gcloud projects add-iam-policy-binding $PROJECT_ID \
      --member="serviceAccount:github-actions-deploy@$PROJECT_ID.iam.gserviceaccount.com" \
      --role="roles/artifactregistry.writer" \
    
    # Cloud Run
    gcloud projects add-iam-policy-binding $PROJECT_ID \
      --member="serviceAccount:github-actions-deploy@$PROJECT_ID.iam.gserviceaccount.com" \
      --role="roles/run.admin" \
    
    # GCE
    gcloud projects add-iam-policy-binding $PROJECT_ID \
      --member="serviceAccount:github-actions-deploy@$PROJECT_ID.iam.gserviceaccount.com" \
      --role="roles/compute.admin" \

    # Service Account User
    gcloud projects add-iam-policy-binding $PROJECT_ID \
      --member="serviceAccount:github-actions-deploy@$PROJECT_ID.iam.gserviceaccount.com" \
      --role="roles/iam.serviceAccountUser"
    ```

4.  **Generate JSON Key**:
    ```bash
    gcloud iam service-accounts keys create gcp-sa-key.json \
      --iam-account=github-actions-deploy@$PROJECT_ID.iam.gserviceaccount.com
    ```
    *Copy the contents of `gcp-sa-key.json` and paste it into the `GCP_SA_KEY` GitHub secret.*

5.  **Clean up local key**:
    ```bash
    rm gcp-sa-key.json
    ```

