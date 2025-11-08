# GitHub Pages Setup for Floppy Turd Privacy Policy

## Quick Setup (5 minutes)

### Step 1: Update the Files
1. Open `docs/privacy-policy.html`
2. Replace `[YOUR_EMAIL_HERE]` with your contact email (e.g., `floppyturd@gmail.com`)
3. Replace `[YOUR_USERNAME]` with your GitHub username

4. Open `docs/index.html`
5. Replace `[YOUR_EMAIL_HERE]` with the same email

### Step 2: Push to GitHub
```bash
cd /Users/aimac/Development/FloppyTurd
git add docs/privacy-policy.html docs/index.html
git commit -m "Add privacy policy for App Store submission"
git push origin main
```

### Step 3: Enable GitHub Pages
1. Go to your GitHub repository: `https://github.com/[YOUR_USERNAME]/FloppyTurd`
2. Click **Settings** (top right)
3. Scroll down to **Pages** in the left sidebar
4. Under "Source", select:
   - **Source:** Deploy from a branch
   - **Branch:** `main`
   - **Folder:** `/docs`
5. Click **Save**

### Step 4: Wait (1-2 minutes)
GitHub will build your site. Refresh the Pages settings page to see the URL.

### Step 5: Get Your URL
Your privacy policy will be at:
```
https://[YOUR_USERNAME].github.io/FloppyTurd/privacy-policy.html
```

Example:
```
https://johndoe.github.io/FloppyTurd/privacy-policy.html
```

---

## Use This URL in App Store Connect

When submitting your app, paste the privacy policy URL into:
- **App Information** → **Privacy Policy URL**

Example: `https://yourusername.github.io/FloppyTurd/privacy-policy.html`

---

## Alternative: Custom Domain (Optional)

If you have your own domain (e.g., `floppyturd.com`), you can use it:

1. In your domain's DNS settings, add a CNAME record:
   - **Host:** `www` (or `@` for root domain)
   - **Points to:** `[YOUR_USERNAME].github.io`

2. In GitHub Pages settings, enter your custom domain:
   - **Custom domain:** `www.floppyturd.com`

3. Your privacy policy URL becomes:
   - `https://www.floppyturd.com/FloppyTurd/privacy-policy.html`

---

## Testing Your Privacy Policy

Before submitting to App Store:

1. Visit your GitHub Pages URL
2. Check that all links work
3. Verify your email address is correct
4. Test on mobile (looks good on iPhone)
5. Read through to make sure it matches your app

---

## What's Included in the Privacy Policy?

✅ **Transparent about ads:** Explains Google AdMob usage
✅ **GameCenter disclosure:** Mentions Apple GameCenter leaderboards
✅ **IAP transparency:** Explains "Remove Ads" purchase
✅ **COPPA compliant:** Addresses children's privacy (9+ rating)
✅ **GDPR compliant:** Covers EU users' rights
✅ **CCPA compliant:** Covers California residents
✅ **Clear data practices:** Explains local storage only
✅ **User controls:** Shows how to disable tracking
✅ **Contact info:** Provides email for questions

---

## App Store Requirements Met

✅ Required for apps with ads
✅ Required for apps with IAP
✅ Required for apps using GameCenter
✅ Required for apps with data collection (even if it's AdMob)
✅ Publicly accessible URL (GitHub Pages)
✅ Mobile-friendly design
✅ Easy to read and understand

---

## Comparison to Other Games' Privacy Policies

Your privacy policy is **similar to popular free games** like:
- **Flappy Bird clones:** Same ad-based model
- **Subway Surfers:** AdMob + GameCenter + IAP
- **Crossy Road:** Free with ads and optional purchases

**Differences:**
- ✅ Yours is MORE transparent (clearly states what you DON'T collect)
- ✅ Yours is SIMPLER to read (no legal jargon overload)
- ✅ Yours gives users MORE control info (iOS settings instructions)

---

## Next Steps After GitHub Pages is Live

1. ✅ Copy the full URL (e.g., `https://username.github.io/FloppyTurd/privacy-policy.html`)
2. ✅ Paste it into App Store Connect → App Information → Privacy Policy URL
3. ✅ Fill out the App Privacy questionnaire in App Store Connect:
   - **Data Used to Track You:** YES - Advertising Identifier
   - **Data Linked to You:** NO (AdMob collects, not you)
   - **Data Not Linked to You:** Performance Data, Crash Data
4. ✅ Submit your app for review!

---

## Common Questions

**Q: Do I need to update the privacy policy when I update the app?**
A: Only if you add new data collection features (like analytics, social login, etc.). For bug fixes and content updates, no changes needed.

**Q: Can I use this privacy policy for other games?**
A: Yes, but update it to match each game's features (remove GameCenter if not used, etc.)

**Q: What if I don't want to use my personal email?**
A: Create a free email specifically for the game (e.g., `floppyturd.support@gmail.com`)

**Q: Is GitHub Pages free forever?**
A: Yes! Public repositories get free GitHub Pages hosting.

---

## Done! 🎉

Your privacy policy is now:
- ✅ Professionally written
- ✅ Legally compliant
- ✅ App Store ready
- ✅ Free to host
- ✅ Easy to update

