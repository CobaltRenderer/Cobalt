<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:doc="http://www.cobaltrenderer.com/schema/XMLDocSchema.xsd">

  <!-- Specify the output document format settings -->
  <xsl:output method="html" indent="yes" encoding="UTF-8" doctype-system="about:legacy-compat"/>
  <xsl:strip-space elements="*"/>

  <!-- Specify input parameters for the transform -->
  <xsl:param name="TypeFilesPresent" />
  <xsl:param name="TypeFilesPresentDoc" />
  <xsl:variable name="TypeFilesPresentResolved">
    <xsl:choose>
      <xsl:when test="$TypeFilesPresentDoc != ''">
        <xsl:value-of select="document($TypeFilesPresentDoc)/TypeFilesPresent/@value"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$TypeFilesPresent"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <!-- Process the root "XMLDocTOC" element -->
  <xsl:template match="/doc:XMLDocTOC">
    <html>
      <head>
        <link href="../styles/DocStyle.css" rel="stylesheet" type="text/css"/>
        <script><![CDATA[
  window.onload = function loadedFunction() { 
  var tree = document.querySelectorAll('ul.tocRoot span:not(:last-child)');
  for(var i = 0; i < tree.length; i++){
    tree[i].addEventListener('click', function(e) {
        var parent = e.target.parentElement;
        var classList = parent.classList;
        if(classList.contains("open")) {
            classList.remove('open');
        } else {
            classList.add('open');
        }
        e.preventDefault();
    });
  }
}]]>
        </script>
      </head>
      <body>
        <div class="mainSection">
          <xsl:apply-templates/>
        </div>
      </body>
    </html>
  </xsl:template>

  <!-- Process "TOCFragment" elements -->
  <xsl:template match="doc:TOCRoot">
    <ul class="tocRoot">
      <xsl:apply-templates/>
    </ul>
  </xsl:template>

  <xsl:template match="doc:TOCFragment">
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="doc:TOCFragmentRef">
    <xsl:apply-templates/>
  </xsl:template>

  <!-- Process "TOCGroup" elements -->
  <xsl:template match="doc:TOCGroup">
    <li class="tocGroupListEntry open">
    <xsl:choose>
      <xsl:when test="not(@OpenByDefault) or @OpenByDefault='true'">
        <xsl:attribute name="class">tocGroupListEntry open</xsl:attribute>
      </xsl:when>
      <xsl:otherwise>
        <xsl:attribute name="class">tocGroupListEntry</xsl:attribute>
      </xsl:otherwise>
    </xsl:choose>
      <span class="tocGroupHeaderNoLink"><xsl:value-of select="@Title"/></span>
      <ul class="tocGroupChildList">
        <xsl:apply-templates/>
      </ul>
    </li>
  </xsl:template>
  <xsl:template match="doc:TOCGroup[@PageName]">
    <li class="tocGroupListEntry">
      <xsl:choose>
        <xsl:when test="contains($TypeFilesPresentResolved,concat('[',@PageName,']'))">
          <a class="tocGroupHeaderWithLink" href="{@PageName}.html" target="docFrame"><xsl:value-of select="@Title"/></a>
        </xsl:when>
        <xsl:otherwise>
          <span class="missinglink"><xsl:value-of select="@Title"/></span>
        </xsl:otherwise>
      </xsl:choose>
      <ul class="tocGroupChildList">
        <xsl:apply-templates/>
      </ul>
    </li>
  </xsl:template>

  <!-- Process "TOCEntry" elements -->
  <xsl:template match="doc:TOCEntry">
    <li class="tocEntry">
      <xsl:choose>
        <xsl:when test="contains($TypeFilesPresentResolved,concat('[',@PageName,']'))">
          <a href="{@PageName}.html" target="docFrame"><xsl:value-of select="@Title"/></a>
        </xsl:when>
        <xsl:otherwise>
          <span class="missinglink"><xsl:value-of select="@Title"/></span>
        </xsl:otherwise>
      </xsl:choose>
    </li>
  </xsl:template>

  <!-- Helper to build class="..." for <img> -->
  <xsl:template name="set-image-classes">
    <!-- existing / base class, e.g. 'missingimagelink' -->
    <xsl:param name="baseClass" select="''"/>
  
    <!-- only output class if something will be inside it -->
    <xsl:if test="$baseClass != '' or @Size or @DarkModeAware">
      <xsl:attribute name="class">
        <!-- base class if one was passed in -->
        <xsl:if test="$baseClass != ''">
          <xsl:value-of select="$baseClass"/>
          <xsl:if test="@Size or @DarkModeAware">
            <xsl:text> </xsl:text>
          </xsl:if>
        </xsl:if>
  
        <!-- size class: imgVerySmall / imgSmall / ... -->
        <xsl:if test="@Size">
          <xsl:text>img</xsl:text>
          <xsl:value-of select="@Size"/>
          <xsl:if test="@DarkModeAware">
            <xsl:text> </xsl:text>
          </xsl:if>
        </xsl:if>
  
        <!-- dark-mode-aware marker -->
        <xsl:if test="@DarkModeAware = 'true' or @DarkModeAware = '1'">
          <xsl:text>DarkModeAware</xsl:text>
        </xsl:if>
      </xsl:attribute>
    </xsl:if>
  </xsl:template>

  <!-- Process "Image" elements -->
  <xsl:template match="doc:Image">
    <img alt="{.}" src="../images/{.}.png"><xsl:call-template name="set-image-classes"/></img>
  </xsl:template>
  <xsl:template match="doc:Image[@PageName]">
    <xsl:choose>
      <xsl:when test="contains($TypeFilesPresentResolved,concat('[',@PageName,']'))">
        <a href="{@PageName}.html"><img alt="{.}" src="../images/{.}.png"><xsl:call-template name="set-image-classes"/></img></a>
      </xsl:when>
      <xsl:otherwise>
        <img alt="{.}" src="../images/{.}.png"><xsl:call-template name="set-image-classes"><xsl:with-param name="baseClass" select="'missingimagelink'"/></xsl:call-template></img>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:stylesheet>
