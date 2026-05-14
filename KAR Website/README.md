# Kustom Auto Radio — Website

## Overview

The KAR website is a production web application built for **Kustom Auto Radio**, a car audio installation shop with over 50 years in business, located in Dodge City, KS. It serves as the business's primary digital presence — replacing an outdated third-party site and giving the shop full ownership over its online identity. The site provides customers with service information, a contact form, curated Google reviews, and business details, while integrating with cloud services for form handling, email notification, and analytics.

**Live Site:** [kustomautoradio.com](https://www.kustomautoradio.com)

---

## Purpose

The site was built to replace the shop's old web.com-managed website, which had no HTTPS, limited flexibility, and no backend integrations. The new platform was designed from the ground up to be fast, secure, and maintainable — with a custom dark/crimson brand identity matching the shop's logo, real backend functionality, and a foundation the owner controls independently of any third-party website builder.

---

## What It Covers

### Customers
- Browse all service offerings — car stereos, speakers, amplifiers, subwoofers, remote starters, security systems, window tint, and accessories
- Submit contact and service inquiry requests directly through the site
- Read curated 5-star Google reviews from verified customers
- Find business location, hours, and an embedded Google Maps view
- Access policy pages (Privacy Policy, Conditions of Use, Accessibility, Notice & Takedown)

### Business Operations
- Contact form submissions are saved to Firestore and trigger email notifications to the shop inbox
- Rate limiting protects the form from abuse (5 submissions per IP per 15 minutes)
- Built-in SEO (sitemap.xml, robots.txt, OpenGraph, Twitter cards, JSON-LD LocalBusiness schema) to improve search visibility
- Google Analytics 4 integration for traffic and engagement tracking
- Custom 404 page with branded navigation back to the site

---

## Operational Value

The website helps the business by:

- Replacing a locked, insecure third-party platform with a fully owned, independently deployed solution
- Eliminating the "Not Secure" browser warning the old site had (automatic HTTPS via Vercel)
- Centralizing customer-facing information for all service lines in one place
- Automating contact form intake with cloud storage and email notification
- Improving organic search presence through structured SEO and schema markup
- Establishing a clean, professional brand presence matching the shop's 50+ year reputation

---

## Platform Summary

The production site was built with **Next.js 16** (App Router) and **TypeScript**, styled with **Tailwind CSS v4**, and deployed on **Vercel** with automatic deploys from GitHub. Backend integrations include **Firebase Admin SDK** (Firestore for form storage), **Resend** (transactional email notifications), and **Google Places API** (live overall rating and review count). The domain is managed through **Cloudflare** with DNS configured for Vercel hosting and Google Workspace email.

---

## Repository Note

This repository does not contain application source code. It exists to provide a public-facing overview of the project and its technical scope for portfolio and documentation purposes.

---

## Copyright

2026 Copyright © Christian Maldonado. All rights reserved.
