/*
 * Copyright (C) 2014 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <QFile>
#include <QImage>
#include <QOpenGLFunctions>
#include "Engine.h"
#include "TextureManager.h"
#include "renderer/Texture.h"
#include "utils/GraphicsUtil.h"
#include "utils/Util.h"

/**
 * Implementation of Texture that owns a raw GL texture.
 *
 * The texture is uploaded lazily on first bind, when a GL context is
 * guaranteed to be current (inside begin/endExternalCommands). Going
 * through QSGTexture/RHI does not work here: RHI never realizes textures
 * that only our external GL commands reference, so nativeTexture() stays 0
 * and the world renders black.
 */
class TextureImpl : public Texture {
public:
    explicit TextureImpl(const QImage& image);
    virtual ~TextureImpl();

    virtual void bind() override;
    virtual void unbind() override;
    virtual int getWidth() const override;
    virtual int getHeight() const override;
    virtual GLint getMagnificationFilter() override;
    virtual void setMagnificationFilter(GLint value) override;
    virtual GLint getMinificationFilter() override;
    virtual void setMinificationFilter(GLint value) override;
    virtual GLint getHorizontalWrapMode() override;
    virtual void setHorizontalWrapMode(GLint value) override;
    virtual GLint getVerticalWrapMode() override;
    virtual void setVerticalWrapMode(GLint value) override;
private:
    void uploadIfNeeded();
    void applyParameters(QOpenGLFunctions* glFuncs);

    QImage mImage;
    GLuint mTextureId = 0;
    bool mUploaded = false;
    bool mParamsDirty = true;
    int mWidth = 0;
    int mHeight = 0;
    GLint mMagFilter = GL_LINEAR;
    GLint mMinFilter = GL_NEAREST_MIPMAP_LINEAR;
    GLint mWrapS = GL_REPEAT;
    GLint mWrapT = GL_REPEAT;
};

TextureManager::~TextureManager() {
}

TexturePtr TextureManager::getTexture(const QString& path) {
    // Check if we have this cached.
    const TexturePtr& texturePtr = mTextures.value(path);
    if (texturePtr) {
        return texturePtr;
    }
    // Check our deleted map too.
    if (!mDeleteQueue.isEmpty()) {
        const TexturePtr& deletedTexturePtr = mDeleteQueue.value(path);
        if (deletedTexturePtr) {
            // Move this back to the active list.
            TexturePtr strongTexturePtr = deletedTexturePtr;
            mTextures[path] = strongTexturePtr;
            mDeleteQueue.remove(path);
            return strongTexturePtr;
        }
    }

    // We don't have it cached. Load it now.
    if (!QFile(path).exists()) {
        // This file doesn't exist.
        // It will load as a blank texture. We could optionally force it to load a "broken"
        // texture to indicate failure. Qt doesn't by default report texture load failures, so
        // manually log it.
        // TODO: Consider replacing with broken texture.
#if defined(QT_DEBUG)
        qDebug() << "Unable to load missing texture:" << path;
#endif
    } else {
#if defined(QT_DEBUG)
        // qDebug() << "Loading texture:" << path;
#endif
    }

    QImage image(path);
    if (image.isNull()) {
        qWarning() << "VoltAir GL null image texture:" << path;
    }
    TexturePtr texture(new TextureImpl(image));
    texture->setHorizontalWrapMode(GL_CLAMP_TO_EDGE);
    texture->setVerticalWrapMode(GL_CLAMP_TO_EDGE);
    mTextures[path] = texture;
    return texture;
}

TexturePtr TextureManager::getTextureAsset(const QString& assetPath) {
    return getTexture(Util::getPathToAsset(assetPath));
}

TexturePtr TextureManager::getTextureImage(const QString& imagePath) {
    return getTexture(Util::getPathToImage(imagePath));
}

void TextureManager::markAllTexturesForDeletion() {
    // Copy out current textures and clear the cache.
    QMap<QString, TexturePtr> toMark;
    std::swap(mTextures, toMark);

    // Clean up the delete queue and prepare it for additional items.
    processDeletion();

    // Place all textures on the delete queue.
    // We'll check if all other references to each of them were released next frame.
    std::swap(mDeleteQueue, toMark);
}

void TextureManager::processDeletion() {
    // Put all textures that are still in use back in the cache.
    for (auto it = mDeleteQueue.begin(); it != mDeleteQueue.end(); ++it) {
        const QString& path = it.key();
        const TexturePtr& texture = it.value();
        if (!texture.unique()) {
            // This texture is now in use (someone else holds a ref to it).
            // Put it back in the cache map.
            mTextures[path] = texture;
            // TODO: commenting out debugging until we figure out correct mechanism.
#if defined(QT_DEBUG)
            // qDebug() << "Retaining texture:" << path;
#endif
        } else {
            // We hold the only reference to this texture, so delete it.
#if defined(QT_DEBUG)
            // qDebug() << "Deleting texture:" << path;
#endif
        }
    }
    mDeleteQueue.clear();
}

TextureImpl::TextureImpl(const QImage& image)
        : mImage(image),
          mWidth(image.width()),
          mHeight(image.height()) {
}

TextureImpl::~TextureImpl() {
    if (mUploaded && mTextureId != 0) {
        // May be a no-op if no context is current; textures are long-lived
        // so a leak here is bounded and preferable to a crash.
        if (QOpenGLFunctions* glFuncs = GraphicsUtil::getOpenGLFunctions()) {
            glFuncs->glDeleteTextures(1, &mTextureId);
        }
        mTextureId = 0;
    }
}

void TextureImpl::uploadIfNeeded() {
    QOpenGLFunctions* glFuncs = GraphicsUtil::getOpenGLFunctions();
    if (!glFuncs) {
        return;
    }
    if (!mUploaded) {
        if (mImage.isNull() || mWidth <= 0 || mHeight <= 0) {
            return;
        }
        QImage upload = mImage.convertToFormat(QImage::Format_RGBA8888);
        glFuncs->glGenTextures(1, &mTextureId);
        glFuncs->glBindTexture(GL_TEXTURE_2D, mTextureId);
        applyParameters(glFuncs);
        glFuncs->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mWidth, mHeight, 0, GL_RGBA,
                GL_UNSIGNED_BYTE, upload.constBits());
        glFuncs->glGenerateMipmap(GL_TEXTURE_2D);
        mImage = QImage();
        mUploaded = true;
        mParamsDirty = false;
    } else {
        glFuncs->glBindTexture(GL_TEXTURE_2D, mTextureId);
        if (mParamsDirty) {
            applyParameters(glFuncs);
            mParamsDirty = false;
        }
    }
}

void TextureImpl::applyParameters(QOpenGLFunctions* glFuncs) {
    glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mMagFilter);
    glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mMinFilter);
    glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mWrapS);
    glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mWrapT);
}

void TextureImpl::bind() {
    uploadIfNeeded();
}

void TextureImpl::unbind() {
    if (QOpenGLFunctions* glFuncs = GraphicsUtil::getOpenGLFunctions()) {
        glFuncs->glBindTexture(GL_TEXTURE_2D, 0);
    }
}

int TextureImpl::getWidth() const {
    return mWidth;
}

int TextureImpl::getHeight() const {
    return mHeight;
}

GLint TextureImpl::getMagnificationFilter() {
    return mMagFilter;
}

void TextureImpl::setMagnificationFilter(GLint value) {
    mMagFilter = value;
    mParamsDirty = true;
}

GLint TextureImpl::getMinificationFilter() {
    return mMinFilter;
}

void TextureImpl::setMinificationFilter(GLint value) {
    mMinFilter = value;
    mParamsDirty = true;
}

GLint TextureImpl::getHorizontalWrapMode() {
    return mWrapS;
}

void TextureImpl::setHorizontalWrapMode(GLint value) {
    mWrapS = value;
    mParamsDirty = true;
}

GLint TextureImpl::getVerticalWrapMode() {
    return mWrapT;
}

void TextureImpl::setVerticalWrapMode(GLint value) {
    mWrapT = value;
    mParamsDirty = true;
}
