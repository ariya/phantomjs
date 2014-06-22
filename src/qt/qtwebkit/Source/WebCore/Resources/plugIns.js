
// Function called from WebCore.
function createOverlay(shadowRoot, titleText, subtitleText)
{
    // Generate the following structure:
    //
    // <div pseudo="-webkit-snapshotted-plugin-content">
    //     <div class="snapshot-overlay" aria-label="[Title]: [Subtitle]" role="button">
    //         <div class="snapshot-label">
    //             <div class="snapshot-title">[Title]</div>
    //             <div class="snapshot-subtitle">[Subtitle]</div>
    //         </div>
    //     </div>
    // </div>

    var shadowContainer = document.createElement("div");
    shadowContainer.setAttribute("pseudo", "-webkit-snapshotted-plugin-content");

    var overlay = shadowContainer.appendChild(document.createElement("div"));
    overlay.setAttribute("aria-label", titleText + ": " + subtitleText);
    overlay.setAttribute("role", "button");
    overlay.className = "snapshot-overlay";

    var snapshotLabel = overlay.appendChild(document.createElement("div"));
    snapshotLabel.className = "snapshot-label";

    var title = snapshotLabel.appendChild(document.createElement("div"));
    title.className = "snapshot-title";
    title.textContent = titleText;

    var subtitle = snapshotLabel.appendChild(document.createElement("div"));
    subtitle.className = "snapshot-subtitle";
    subtitle.textContent = subtitleText;

    shadowRoot.appendChild(shadowContainer);
};
