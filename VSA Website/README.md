# Vernon's Signature Audio — Website

## Overview

The VSA website is a production web application built for a full-service automotive audio, tint, paint protection, and custom installation shop in Lubbock, TX. It serves as the primary digital presence for the business — providing customers with service information, a contact and appointment request form, gallery of past builds, customer reviews, and access to the VSA Rewards app. It also integrates with third-party platforms for email, analytics, video content, and financing.

This public repository is intentionally documentation-only. The application source code is private and maintained separately as proprietary business software.

**Live Site:** Live Site: vernonsignatureaudio.com

---

## Purpose

The site was designed to replace an outdated Hibu-managed website and give the business full ownership over its digital presence. Rather than relying on a third-party website builder with limited flexibility, the platform was built from the ground up to reflect the quality and professionalism of the shop — with fast load times, modern design, and real integrations that support daily operations.

---

## What It Covers

### Customers
- Browse all services offered (audio, tint, fabrication, PPF, remote start, accessories, interlock)
- Submit appointment and service consultation requests directly through the site
- View a gallery of real customer builds
- Read live Google Reviews pulled directly from the business profile
- Download the VSA Rewards app and track points toward service discounts

### Business Operations
- Contact form submissions are saved to Firestore and trigger email notifications to the shop
- Rate limiting protects the form from spam
- Built-in SEO (sitemap, robots.txt, OpenGraph, JSON-LD structured data) to improve search visibility
- Google Analytics 4 integration for traffic and engagement tracking

### Content
- VSA TV page surfacing build videos and podcast content from YouTube
- Snap Finance promotional section for financing-eligible customers
- Award badges and certifications displayed in the navigation

---

## Operational Value

The website helps the business by:

- Replacing a locked third-party platform with a fully owned, independently deployed solution
- Centralizing customer-facing information across all service lines
- Automating contact form intake with cloud storage and email notification
- Improving organic search presence through structured SEO implementation
- Providing a foundation for future integrations (scheduling, promotions, loyalty program)

---

## Platform Summary

The production site was built with **Next.js** (App Router) and **TypeScript**, styled with **Tailwind CSS v4**, and deployed on **Vercel** with automatic deploys from GitHub. Backend integrations include **Firebase Admin SDK** (Firestore), **Resend** (transactional email), **Google Places API** (live reviews), and **YouTube Data API v3** (VSA TV). The domain is managed through **Cloudflare** with DNS configured for Vercel hosting and Microsoft 365 email.

---

## Repository Note

This repository does not contain application source code. It exists to provide a public-facing overview of the project and its technical scope for portfolio and documentation purposes.

---

## Copyright

2026 Copyright © Christian Maldonado. All rights reserved.

This application and its associated software, assets, and documentation are proprietary and protected as business intellectual property. No source code is published in this repository.
