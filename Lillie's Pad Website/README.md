# The Lillie's Pad — Website

## Overview

The Lillie's Pad website is a production web application built for a privately owned vacation cabin rental in Ruidoso, New Mexico. It serves as the primary digital presence for the property — providing guests with property details, a full photo gallery, house rules and amenities, customer reviews, and a booking inquiry form with real-time availability checking. It integrates with third-party platforms for email delivery, calendar sync, and cloud data storage.

This public repository is intentionally documentation-only. The application source code is private and maintained separately as proprietary business software.

**Live Site:** [thelilliespad.com](https://www.thelilliespad.com)

---

## Purpose

The site was built to give the property owners full control over their digital presence outside of Airbnb and Booking.com. Rather than relying solely on third-party listing platforms, the platform presents the cabin professionally and independently — with fast load times, a modern design, and real integrations that support the booking workflow.

---

## What It Covers

### Guests
- Browse the full property overview including specs (5 bed / 4 bath / sleeps 10)
- View a categorized photo gallery with full-screen lightbox viewing
- Review all amenities across 6 categories and complete house rules
- Read guest reviews
- Submit a booking inquiry with automatic availability conflict detection against live Airbnb and Booking.com calendars
- Direct links to book on Airbnb and Booking.com

### Business Operations
- Contact/booking form submissions are saved to Firestore and trigger email notifications to the property owners
- Rate limiting protects the form from abuse (5 requests per 15 minutes per IP)
- iCal calendar sync pulls blocked dates from Airbnb and Booking.com feeds and surfaces conflicts in the booking form in real time
- Legal policy pages (Privacy Policy, Conditions of Use, Notice & Take Down, Accessibility) included for compliance
- Built-in SEO (sitemap, robots.txt, OpenGraph, JSON-LD LodgingBusiness structured data) to improve search visibility

---

## Operational Value

The website helps the business by:

- Establishing a standalone web presence independent of rental platform algorithms
- Automating booking inquiry intake with cloud storage and email notification
- Surfacing real-time calendar availability to reduce double-booking risk
- Improving organic search presence through structured SEO implementation
- Presenting the property with a professional, custom-designed experience

---

## Platform Summary

The production site was built with **Next.js 15** (App Router) and **TypeScript**, styled with **Tailwind CSS v4**, and deployed on **Vercel** with automatic deploys from GitHub. Backend integrations include **Firebase Admin SDK** (Firestore for form submissions), **Resend** (transactional email), and **iCal feed parsing** (Airbnb + Booking.com calendar sync). The domain is managed through **Vercel DNS** with SPF, DKIM, and DMARC records configured for Resend email delivery.

---

## Repository Note

This repository does not contain application source code. It exists to provide a public-facing overview of the project and its technical scope for portfolio and documentation purposes.

---

## Copyright

2026 Copyright © Christian Maldonado. All rights reserved.

This application and its associated software, assets, and documentation are proprietary and protected as business intellectual property. No source code is published in this repository.
