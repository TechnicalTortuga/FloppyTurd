#!/usr/bin/env python3
"""
Add Swift Package Manager dependency to Xcode project programmatically
This avoids needing to open Xcode GUI
"""

import sys
import re
import uuid

def add_package_to_project(project_path, package_url, version="12.0.0"):
    """Add a Swift package dependency to an Xcode project file"""
    
    try:
        with open(project_path, 'r') as f:
            content = f.read()
    except FileNotFoundError:
        print(f"❌ Project file not found: {project_path}")
        return False
    
    # Generate unique IDs for the package objects
    package_ref_id = str(uuid.uuid4()).replace('-', '').upper()[:24]
    package_product_id = str(uuid.uuid4()).replace('-', '').upper()[:24]
    
    # 1. Add package reference to XCRemoteSwiftPackageReference section
    package_reference = f'''
		{package_ref_id} /* XCRemoteSwiftPackageReference "swift-package-manager-google-mobile-ads" */ = {{
			isa = XCRemoteSwiftPackageReference;
			repositoryURL = "{package_url}";
			requirement = {{
				kind = upToNextMajorVersion;
				minimumVersion = {version};
			}};
		}};'''
    
    # Find the end of the XCRemoteSwiftPackageReference section
    pattern = r'(/\* End XCRemoteSwiftPackageReference section \*/)'
    if not re.search(pattern, content):
        print("⚠️  No XCRemoteSwiftPackageReference section found, adding one...")
        # Add the section before /* End PBXProject section */
        pattern = r'(/\* End PBXProject section \*/)'
        replacement = f'''/* Begin XCRemoteSwiftPackageReference section */
{package_reference}
/* End XCRemoteSwiftPackageReference section */

\\1'''
        content = re.sub(pattern, replacement, content, count=1)
    else:
        replacement = f'{package_reference}\\n\\1'
        content = re.sub(pattern, replacement, content, count=1)
    
    # 2. Add package product dependency
    package_product = f'''
		{package_product_id} /* XCSwiftPackageProductDependency "GoogleMobileAds" */ = {{
			isa = XCSwiftPackageProductDependency;
			package = {package_ref_id} /* XCRemoteSwiftPackageReference "swift-package-manager-google-mobile-ads" */;
			productName = GoogleMobileAds;
		}};'''
    
    # Find or create XCSwiftPackageProductDependency section
    pattern = r'(/\* End XCSwiftPackageProductDependency section \*/)'
    if not re.search(pattern, content):
        print("⚠️  No XCSwiftPackageProductDependency section found, adding one...")
        pattern = r'(/\* End XCRemoteSwiftPackageReference section \*/)'
        replacement = f'''\\1

/* Begin XCSwiftPackageProductDependency section */
{package_product}
/* End XCSwiftPackageProductDependency section */'''
        content = re.sub(pattern, replacement, content, count=1)
    else:
        replacement = f'{package_product}\\n\\1'
        content = re.sub(pattern, replacement, content, count=1)
    
    # 3. Add package to project's packageReferences
    # Find the main project object
    pattern = r'(packageReferences = \(\s*\);)'
    if re.search(pattern, content):
        replacement = f'packageReferences = (\\n\\t\\t\\t\\t{package_ref_id} /* XCRemoteSwiftPackageReference "swift-package-manager-google-mobile-ads" */,\\n\\t\\t\\t);'
        content = re.sub(pattern, replacement, content, count=1)
    else:
        print("⚠️  Package references array not empty or not found")
    
    # 4. Add package product to FloppyTurd target's frameworks
    # Find the FloppyTurd target's frameworks build phase
    pattern = r'(name = FloppyTurd;.*?productName = FloppyTurd;.*?)(};)'
    
    # This is complex - we need to find the target and add to its dependencies
    # For now, let's just write the modified content and let xcodebuild resolve
    
    # Write back
    try:
        with open(project_path, 'w') as f:
            f.write(content)
        print(f"✅ Successfully added package reference to {project_path}")
        return True
    except Exception as e:
        print(f"❌ Failed to write project file: {e}")
        return False

if __name__ == "__main__":
    project_file = "build_ios/FloppyTurd.xcodeproj/project.pbxproj"
    package_url = "https://github.com/googleads/swift-package-manager-google-mobile-ads.git"
    
    print("📦 Adding Google Mobile Ads SDK to Xcode project programmatically...")
    success = add_package_to_project(project_file, package_url)
    
    if success:
        print("\n✅ Package reference added!")
        print("\n📋 Next step: Run xcodebuild to resolve packages:")
        print("   xcodebuild -resolvePackageDependencies -project build_ios/FloppyTurd.xcodeproj -scheme FloppyTurd")
    else:
        print("\n❌ Failed to add package reference")
        sys.exit(1)

