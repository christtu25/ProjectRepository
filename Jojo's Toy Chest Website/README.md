# Jojo's Toy Chest — Website

## Overview

The Jojo's Toy Chest website is a production e-commerce web application built for a pop culture toy and collectibles store based in Lubbock, Texas — run by a YouTuber who sells Transformers, MOTU, DC, action figures, and more. The platform serves as the store's primary digital presence, providing customers with a fully functional online shopping experience including a live product catalog, cart system, Stripe checkout, customer order tracking, and an admin management dashboard.

This public repository is intentionally documentation-only. The application source code is private and maintained separately as proprietary business software.

**Live Site:** [jojostoychest.com](https://www.jojostoychest.com)

---

## Purpose

The site was built to give the store owner a complete, self-managed e-commerce platform — independent of marketplace fees and restrictions. Rather than relying solely on platforms like eBay or Etsy, the store presents its catalog professionally with a custom brand identity, full inventory control, real payment processing, and direct customer communication.

---

## What It Covers

### Customers
- Browse the full product catalog with category filtering (Transformers, MOTU, DC, Action Figures, Collectibles, Other) and live search
- View individual product details including images, description, price, and stock availability
- Add items to a persistent cart backed by Firestore — cart is saved per account and restored on login
- Real-time stock validation via Firestore listeners — out-of-stock items are automatically removed from carts the moment stock reaches zero, across all active sessions
- Secure checkout via Stripe — server-side price and stock validation ensures client-submitted data is never trusted
- Email and password authentication with email verification
- Order dashboard with four status tabs: Incoming (processing), Active (shipped), Completed (delivered), and History (cancelled)
- Shipped orders display clickable carrier tracking links (UPS, FedEx, USPS)
- Submit and view customer reviews (account required, pending admin approval)
- Contact form with cloud storage and owner email notification
- YouTube page showcasing the owner's latest videos (with Shorts automatically filtered out)

### Business Operations
- Stripe webhook receives payment confirmation and automatically decrements product stock in Firestore for each purchased item
- Webhook scans all user carts after a purchase and removes or caps any items whose stock was reduced — preventing other customers from attempting to purchase unavailable quantities
- Order confirmation emails sent to customers via Resend
- Contact form submissions saved to Firestore and trigger email notifications to the store owner
- Rate limiting on form submissions (5 requests per 15 minutes per IP) protects against abuse
- Full admin product management dashboard — add, edit, and delete products directly from the account page
- In-browser image crop and zoom tool for product photo uploads — final image stored on Cloudinary
- Review moderation page — approve or reject pending customer reviews before they appear publicly
- Firebase Auth custom email domain configured via Namecheap DNS — verification emails delivered from the store's own domain rather than a Firebase subdomain

---

## Operational Value

The platform supports the business by:

- Providing a complete, branded e-commerce experience independent of third-party marketplace algorithms
- Automating inventory management — stock decrements and cart cleanup happen automatically after every purchase with no manual intervention
- Protecting against overselling with server-side validation on every checkout
- Delivering a real-time shopping experience — customers see live stock availability without refreshing
- Centralizing order management, customer reviews, product listings, and contact inquiries in one platform

---

## Design

The visual identity for Jojo's Toy Chest was developed in collaboration with **Khloe Hudgins**, a graphic design student at Texas Tech University. Khloe designed the logo suite and defined the overall color palette and brand aesthetic — capturing the bold, playful energy of pop culture collectibles while maintaining a clean, modern storefront feel. Her work established the visual foundation across the entire platform, from the homepage hero to the product catalog and cart experience.

---

## Platform Summary

The production site was built with **Next.js 16** (App Router, TypeScript) and styled with **Tailwind CSS v4**, deployed on **Vercel** with automatic deploys from GitHub. Backend integrations include **Firebase** (Auth for customer accounts, Firestore for products, orders, carts, reviews, and contacts — accessed via Admin SDK server-side and the client SDK for real-time cart listeners), **Cloudinary** (product image hosting with in-browser crop via react-easy-crop), **Stripe** (checkout sessions, webhook event handling, stock management), and **Resend** (transactional order confirmation and contact notification emails). The domain is managed through **Namecheap DNS** with SPF, DKIM, and custom Firebase Auth email domain records configured for reliable email delivery.

---

## Repository Note

This repository does not contain application source code. It exists to provide a public-facing overview of the project and its technical scope for portfolio and documentation purposes.

---

## Copyright

2026 Copyright © Christian Maldonado. All rights reserved.

This application and its associated software, assets, and documentation are proprietary and protected as business intellectual property. No source code is published in this repository.
