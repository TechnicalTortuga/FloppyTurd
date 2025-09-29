#!/bin/bash

echo "=== SAFE Storage Cleanup Script ==="
echo "Focuses ONLY on user data that can be safely removed"
echo ""

echo "=== CURRENT DISK USAGE ==="
df -h /
echo ""

echo "=== SAFE-TO-CLEAN DIRECTORIES ==="
echo ""

echo "🎯 AI MODELS & CACHES (Safest to clean):"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo "1. HuggingFace Cache: $(du -sh ~/.cache/huggingface/ 2>/dev/null | cut -f1 || echo 'Not found')"
echo "   Location: ~/.cache/huggingface/"
echo "   Safe to delete: YES (models will redownload if needed)"
echo ""

echo "2. LMStudio Models: $(du -sh ~/.lmstudio/models/ 2>/dev/null | cut -f1 || echo 'Not found')"
echo "   Location: ~/.lmstudio/models/"
echo "   Safe to delete: YES (delete unused models only)"
echo ""

echo "=== LARGEST AI MODELS (Safe to delete) ==="
echo ""

echo "🔥 TOP LMSTUDIO MODELS:"
du -sh ~/.lmstudio/models/* 2>/dev/null | sort -hr | head -5 | while read size dir; do
    model_name=$(basename "$dir")
    echo "   $size - $model_name"
done
echo ""

echo "🔥 TOP HUGGINGFACE MODELS:"
du -sh ~/.cache/huggingface/hub/models--* 2>/dev/null | sort -hr | head -5 | while read size dir; do
    model_name=$(basename "$dir" | sed 's/models--//' | sed 's/--/-/g')
    echo "   $size - $model_name"
done
echo ""

echo "🎯 DEVELOPMENT & APPLICATION CACHES:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo "3. Xcode DerivedData: $(du -sh ~/Library/Developer/Xcode/DerivedData/ 2>/dev/null | cut -f1 || echo 'Not found')"
echo "   Location: ~/Library/Developer/Xcode/DerivedData/"
echo "   Safe to delete: YES (will rebuild automatically)"
echo ""

echo "4. Xcode Archives: $(du -sh ~/Library/Developer/Xcode/Archives/ 2>/dev/null | cut -f1 || echo 'Not found')"
echo "   Location: ~/Library/Developer/Xcode/Archives/"
echo "   Safe to delete: YES (old app archives)"
echo ""

echo "5. Application Caches: $(du -sh ~/Library/Caches/ 2>/dev/null | cut -f1 || echo 'Not found')"
echo "   Location: ~/Library/Caches/"
echo "   Safe to delete: MOSTLY (some app caches are safe)"
echo ""

echo "🎯 USER DOCUMENTS & DOWNLOADS:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo "6. Downloads Folder: $(du -sh ~/Downloads/ 2>/dev/null | cut -f1 || echo 'Not found')"
echo "   Location: ~/Downloads/"
echo "   Safe to delete: YES (move important files first)"
echo ""

echo "7. Desktop Files: $(du -sh ~/Desktop/ 2>/dev/null | cut -f1 || echo 'Not found')"
echo "   Location: ~/Desktop/"
echo "   Safe to delete: YES (organize/move important files)"
echo ""

echo "🎯 LARGE APPLICATIONS:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo "8. Steam Games: $(du -sh ~/Library/Application\ Support/Steam/steamapps/common/ 2>/dev/null | cut -f1 || echo 'Not found')"
echo "   Location: ~/Library/Application Support/Steam/steamapps/common/"
echo "   Safe to delete: YES (uninstall games you don't play)"
echo ""

echo "9. Docker: $(du -sh ~/Library/Containers/com.docker.docker/ 2>/dev/null | cut -f1 || echo 'Not found')"
echo "   Location: ~/Library/Containers/com.docker.docker/"
echo "   Safe to delete: CAUTION (stop Docker first, may lose containers)"
echo ""

echo "=== CLEANUP COMMANDS (Copy & Run Individually) ==="
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo "🎯 SAFEST CLEANUP (49GB):"
echo "rm -rf ~/.cache/huggingface/hub/*"
echo ""

echo "🎯 LMSTUDIO MODELS (Remove unused ones):"
echo "rm -rf ~/.lmstudio/models/lmstudio-community    # 12GB"
echo "rm -rf ~/.lmstudio/models/TheBloke             # 8.9GB"
echo "rm -rf ~/.lmstudio/models/Kijai                # 5.7GB"
echo "rm -rf ~/.lmstudio/models/Mungert              # 5.2GB"
echo ""

echo "🎯 XCODE CACHE (5-10GB):"
echo "rm -rf ~/Library/Developer/Xcode/DerivedData/*"
echo "rm -rf ~/Library/Developer/Xcode/Archives/*"
echo ""

echo "🎯 APPLICATION CACHES (Safe ones):"
echo "rm -rf ~/Library/Caches/com.apple.dt.Xcode/*"
echo "rm -rf ~/Library/Caches/com.apple.dt.*"
echo ""

echo "=== VERIFICATION COMMANDS ==="
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo "After cleanup, check results:"
echo "df -h /"
echo "du -sh ~/.cache/huggingface/ 2>/dev/null || echo 'HuggingFace cleaned'"
echo "du -sh ~/.lmstudio/models/ 2>/dev/null || echo 'LMStudio checked'"
echo ""

echo "=== ⚠️  SAFETY NOTES ==="
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "• Run commands ONE AT A TIME"
echo "• AI models will redownload if needed"
echo "• Xcode will rebuild caches automatically"
echo "• Backup important Downloads/Desktop files first"
echo "• NEVER delete system files in /System or /Library/Developer/CoreSimulator"
echo ""

echo "=== 📊 EXPECTED SAVINGS ==="
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "• HuggingFace cache: ~49GB"
echo "• LMStudio models: ~25GB"
echo "• Xcode cache: ~10GB"
echo "• App caches: ~5GB"
echo "• Total potential: ~90GB+"
echo ""

echo "Script completed. Copy and run the commands above individually."